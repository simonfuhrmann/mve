/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <string>
#include <cstring>
#include <cerrno>
#include <cstdlib>

#include "math/octree_tools.h"
#include "util/system.h"
#include "util/arguments.h"
#include "util/tokenizer.h"
#include "mve/depthmap.h"
#include "mve/mesh_info.h"
#include "mve/mesh_io.h"
#include "mve/mesh_io_ply.h"
#include "mve/mesh_tools.h"
#include "mve/scene.h"
#include "mve/view.h"

struct AppSettings
{
    std::string scenedir;
    std::string outmesh;
    std::string dmname = "depth-L0";
    std::string image = "undistorted";
    std::string mask;
    std::string aabb;
    bool with_normals = false;
    bool with_scale = false;
    bool with_conf = false;
    bool poisson_normals = false;
    float min_valid_fraction = 0.0f;
    float scale_factor = 2.5f; /* "Radius" of MVS patch (usually 5x5). */
    std::vector<int> ids;
    bool output_correspondence = false;
};

struct CorrespondenceViewMetadata
{
    unsigned int view_id; 
    unsigned int width; 
    unsigned int height; 
    unsigned long first_idx;
};

struct CorrespondenceData
{
    std::vector<math::Vec2ui> data;
    std::vector<CorrespondenceViewMetadata> metadata;
};

void
append_correspondence_data_from_view(const mve::Image<unsigned int>& vertex_ids, const int view_id, CorrespondenceData* corr_data)
{   
    const unsigned int width = vertex_ids.width();
    CorrespondenceViewMetadata curr_view_metadata;
    curr_view_metadata.view_id = view_id;
    curr_view_metadata.width = width;
    curr_view_metadata.height = vertex_ids.height();
    curr_view_metadata.first_idx = corr_data->data.size();
    corr_data->metadata.push_back(curr_view_metadata);
    
    const unsigned int pixel_amount = vertex_ids.get_pixel_amount();
    const unsigned int filled_pixel_amount = pixel_amount - 
        std::count(vertex_ids.get_data().begin(), vertex_ids.get_data().end(), MATH_MAX_UINT);
    std::vector<math::Vec2ui> curr_view_data (filled_pixel_amount);
    for (unsigned int i = 0; i < pixel_amount; ++i)
        if (vertex_ids.at(i) != MATH_MAX_UINT)
            curr_view_data[vertex_ids.at(i)] = math::Vec2ui(i%width,int(i/width));
    corr_data->data.insert(corr_data->data.end(), curr_view_data.begin(), curr_view_data.end());
}

void
save_correspondence_data(const CorrespondenceData& corr_data, const AppSettings& conf)
{
    std::string corr_data_fname = conf.outmesh + "_correspondence-data.csv";
    std::string corr_metadata_fname = conf.outmesh + "_correspondence-metadata.csv";

    std::ofstream corr_data_file(corr_data_fname);
    std::ofstream corr_metadata_file(corr_metadata_fname);

    if (!corr_data_file.good() || !corr_metadata_file.good())
    {
        std::cerr << "Error: Could not open correspondence file(s)." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    corr_data_file << "x, y\n";
    for (unsigned int i = 0; i < corr_data.data.size(); ++i)
    {
        corr_data_file << corr_data.data[i][0] << ", " <<
            corr_data.data[i][1] << "\n" ;
    }

    corr_metadata_file << "View_ID, Width, Height, First_Vertex_Index\n";
    for (unsigned int i = 0; i < corr_data.metadata.size(); ++i)
    {
        corr_metadata_file << corr_data.metadata[i].view_id << ", " <<
            corr_data.metadata[i].width << ", " <<
            corr_data.metadata[i].height << ", " <<
            corr_data.metadata[i].first_idx << "\n" ;
    }

    corr_data_file.close();
    corr_metadata_file.close();
}

void
poisson_scale_normals (mve::TriangleMesh::ConfidenceList const& confs,
    mve::TriangleMesh::NormalList* normals)
{
    if (confs.size() != normals->size())
        throw std::invalid_argument("Invalid confidences or normals");
    for (std::size_t i = 0; i < confs.size(); ++i)
        normals->at(i) *= confs[i];
}

void
aabb_from_string (std::string const& str,
    math::Vec3f* aabb_min, math::Vec3f* aabb_max)
{
    util::Tokenizer tok;
    tok.split(str, ',');
    if (tok.size() != 6)
    {
        std::cerr << "Error: Invalid AABB given" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 3; ++i)
    {
        (*aabb_min)[i] = tok.get_as<float>(i);
        (*aabb_max)[i] = tok.get_as<float>(i + 3);
    }
    std::cout << "Using AABB: (" << (*aabb_min) << ") / ("
        << (*aabb_max) << ")" << std::endl;
}

int
main (int argc, char** argv)
{
    util::system::register_segfault_handler();
    util::system::print_build_timestamp("MVE Scene to Pointset");

    /* Setup argument parser. */
    util::Arguments args;
    args.set_exit_on_error(true);
    args.set_nonopt_minnum(2);
    args.set_nonopt_maxnum(2);
    args.set_helptext_indent(25);
    args.set_usage("Usage: scene2pset [ OPTS ] SCENE_DIR MESH_OUT");
    args.set_description(
        "Generates a pointset from the scene by projecting reconstructed "
        "depth values in the world coordinate system.");
    args.add_option('d', "depthmap", true, "Name of depth map to use [depth-L0]");
    args.add_option('i', "image", true, "Name of color image to use [undistorted]");
    args.add_option('n', "with-normals", false, "Write points with normals (PLY only)");
    args.add_option('s', "with-scale", false, "Write points with scale values (PLY only)");
    args.add_option('c', "with-conf", false, "Write points with confidence (PLY only)");
    args.add_option('m', "mask", true, "Name of mask/silhouette image to clip 3D points []");
    args.add_option('v', "views", true, "View IDs to use for reconstruction [all]");
    args.add_option('b', "bounding-box", true, "Six comma separated values used as AABB.");
    args.add_option('f', "min-fraction", true, "Minimum fraction of valid depth values [0.0]");
    args.add_option('p', "poisson-normals", false, "Scale normals according to confidence");
    args.add_option('S', "scale-factor", true, "Factor for computing scale values [2.5]");
    args.add_option('C', "correspondence", false, 
        "Output correspondences (in absence of -m and -b only)");
    args.add_option('F', "fssr", true, "FSSR output, sets -nsc and -di with scale ARG");

    args.parse(argc, argv);

    /* Init default settings. */
    AppSettings conf;
    conf.scenedir = args.get_nth_nonopt(0);
    conf.outmesh = args.get_nth_nonopt(1);
    /* Scan arguments. */
    while (util::ArgResult const* arg = args.next_result())
    {
        if (arg->opt == nullptr)
            continue;

        switch (arg->opt->sopt)
        {
            case 'n': conf.with_normals = true; break;
            case 's': conf.with_scale = true; break;
            case 'c': conf.with_conf = true; break;
            case 'd': conf.dmname = arg->arg; break;
            case 'i': conf.image = arg->arg; break;
            case 'm': conf.mask = arg->arg; break;
            case 'v': args.get_ids_from_string(arg->arg, &conf.ids); break;
            case 'b': conf.aabb = arg->arg; break;
            case 'f': conf.min_valid_fraction = arg->get_arg<float>(); break;
            case 'p': conf.poisson_normals = true; break;
            case 'S': conf.scale_factor = arg->get_arg<float>(); break;
            case 'C': conf.output_correspondence = true; break;
            case 'F':
            {
                conf.with_conf = true;
                conf.with_normals = true;
                conf.with_scale = true;
                int const scale = arg->get_arg<int>();
                conf.dmname = "depth-L" + util::string::get<int>(scale);
                conf.image = (scale == 0)
                    ? "undistorted"
                    : "undist-L" + util::string::get<int>(scale);
                break;
            }

            default: throw std::runtime_error("Unknown option");
        }
    }

    if (util::string::right(conf.outmesh, 5) == ".npts"
        || util::string::right(conf.outmesh, 6) == ".bnpts")
    {
        conf.with_normals = true;
        conf.with_scale = false;
        conf.with_conf = false;
    }

    if (conf.poisson_normals)
    {
        conf.with_normals = true;
        conf.with_conf = true;
    }

    /* If requested, use given AABB. */
    math::Vec3f aabbmin, aabbmax;
    if (!conf.aabb.empty())
        aabb_from_string(conf.aabb, &aabbmin, &aabbmax);

    std::cout << "Using depthmap \"" << conf.dmname
        << "\" and color image \"" << conf.image << "\"" << std::endl;

    /* Prepare output mesh. */
    mve::TriangleMesh::Ptr pset(mve::TriangleMesh::create());
    mve::TriangleMesh::VertexList& verts(pset->get_vertices());
    mve::TriangleMesh::NormalList& vnorm(pset->get_vertex_normals());
    mve::TriangleMesh::ColorList& vcolor(pset->get_vertex_colors());
    mve::TriangleMesh::ValueList& vvalues(pset->get_vertex_values());
    mve::TriangleMesh::ConfidenceList& vconfs(pset->get_vertex_confidences());

    /* Load scene. */
    mve::Scene::Ptr scene = mve::Scene::create(conf.scenedir);

    /* Iterate over views and get points. */
    mve::Scene::ViewList& views(scene->get_views());

    /* Prepare correspondence data */
    CorrespondenceData corr_data;

#pragma omp parallel for schedule(dynamic)
    for (std::size_t i = 0; i < views.size(); ++i)
    {
        mve::View::Ptr view = views[i];
        if (view == nullptr)
            continue;

        std::size_t view_id = view->get_id();
        if (!conf.ids.empty() && std::find(conf.ids.begin(),
            conf.ids.end(), view_id) == conf.ids.end())
            continue;

        mve::CameraInfo const& cam = view->get_camera();
        if (cam.flen == 0.0f)
            continue;

        mve::FloatImage::Ptr dm = view->get_float_image(conf.dmname);
        if (dm == nullptr)
            continue;

        if (conf.min_valid_fraction > 0.0f)
        {
            float num_total = static_cast<float>(dm->get_value_amount());
            float num_recon = 0;
            for (int j = 0; j < dm->get_value_amount(); ++j)
                if (dm->at(j) > 0.0f)
                    num_recon += 1.0f;
            float fraction = num_recon / num_total;
            if (fraction < conf.min_valid_fraction)
            {
                std::cout << "View " << view->get_name() << ": Fill status "
                    << util::string::get_fixed(fraction * 100.0f, 2)
                    << "%, skipping." << std::endl;
                continue;
            }
        }

        mve::ByteImage::Ptr ci;
        if (!conf.image.empty())
            ci = view->get_byte_image(conf.image);

#pragma omp critical
        std::cout << "Processing view \"" << view->get_name()
            << "\"" << (ci != nullptr ? " (with colors)" : "")
            << "..." << std::endl;

        /* Triangulate depth map. */
        mve::TriangleMesh::Ptr mesh;

        mve::Image<unsigned int> vertex_ids;
        mesh = mve::geom::depthmap_triangulate(dm, ci, cam, mve::geom::DD_FACTOR_DEFAULT, &vertex_ids); 

        mve::TriangleMesh::VertexList const& mverts(mesh->get_vertices());
        mve::TriangleMesh::NormalList const& mnorms(mesh->get_vertex_normals());
        mve::TriangleMesh::ColorList const& mvcol(mesh->get_vertex_colors());
        mve::TriangleMesh::ConfidenceList& mconfs(mesh->get_vertex_confidences());

        if (conf.with_normals)
            mesh->ensure_normals();

        /* If confidence is requested, compute it. */
        if (conf.with_conf)
        {
            /* Per-vertex confidence down-weighting boundaries. */
            mve::geom::depthmap_mesh_confidences(mesh, 4);

#if 0
            /* Per-vertex confidence based on normal-viewdir dot product. */
            mesh->ensure_normals();
            math::Vec3f campos;
            cam.fill_camera_pos(*campos);
            for (std::size_t i = 0; i < mverts.size(); ++i)
                mconfs[i] *= (campos - mverts[i]).normalized().dot(mnorms[i]);
#endif
        }

        if (conf.poisson_normals)
            poisson_scale_normals(mconfs, &mesh->get_vertex_normals());

        /* If scale is requested, compute it. */
        std::vector<float> mvscale;
        if (conf.with_scale)
        {
            mvscale.resize(mverts.size(), 0.0f);
            mve::MeshInfo mesh_info(mesh);
            for (std::size_t j = 0; j < mesh_info.size(); ++j)
            {
                mve::MeshInfo::VertexInfo const& vinf = mesh_info[j];
                for (std::size_t k = 0; k < vinf.verts.size(); ++k)
                    mvscale[j] += (mverts[j] - mverts[vinf.verts[k]]).norm();
                mvscale[j] /= static_cast<float>(vinf.verts.size());
                mvscale[j] *= conf.scale_factor;
            }
        }

        /* Add vertices and optional colors and normals to mesh. */
        if (conf.aabb.empty())
        {
            /* Fast if no bounding box is given. */
#pragma omp critical
            {
                verts.insert(verts.end(), mverts.begin(), mverts.end());
                if (!mvcol.empty())
                    vcolor.insert(vcolor.end(), mvcol.begin(), mvcol.end());
                if (conf.with_normals)
                    vnorm.insert(vnorm.end(), mnorms.begin(), mnorms.end());
                if (conf.with_scale)
                    vvalues.insert(vvalues.end(), mvscale.begin(), mvscale.end());
                if (conf.with_conf)
                    vconfs.insert(vconfs.end(), mconfs.begin(), mconfs.end());
                if (conf.output_correspondence && conf.mask.empty())
                    append_correspondence_data_from_view(vertex_ids, view->get_id(), &corr_data);
            }
        }
        else
        {
            /* Check every point if a bounding box is given.  */
            for (std::size_t i = 0; i < mverts.size(); ++i)
            {
                if (!math::geom::point_box_overlap(mverts[i], aabbmin, aabbmax))
                    continue;

#pragma omp critical
                {
                    verts.push_back(mverts[i]);
                    if (!mvcol.empty())
                        vcolor.push_back(mvcol[i]);
                    if (conf.with_normals)
                        vnorm.push_back(mnorms[i]);
                    if (conf.with_scale)
                        vvalues.push_back(mvscale[i]);
                    if (conf.with_conf)
                        vconfs.push_back(mconfs[i]);
                }
            }
        }

        dm.reset();
        ci.reset();
        view->cache_cleanup();
    }

    /* If a mask is given, clip vertices with the masks in all images. */
    if (!conf.mask.empty())
    {
        std::cout << "Filtering points using silhouette masks..." << std::endl;
        std::vector<bool> delete_list(verts.size(), false);
        std::size_t num_filtered = 0;

        for (std::size_t i = 0; i < views.size(); ++i)
        {
            mve::View::Ptr view = views[i];
            if (view == nullptr || view->get_camera().flen == 0.0f)
                continue;
            mve::ByteImage::Ptr mask = view->get_byte_image(conf.mask);
            if (mask == nullptr)
            {
                std::cout << "Mask not found for image \""
                    << view->get_name() << "\", skipping." << std::endl;
                continue;
            }
            if (mask->channels() != 1)
            {
                std::cout << "Expected 1-channel mask for image \""
                    << view->get_name() << "\", skipping." << std::endl;
                continue;
            }

            std::cout << "Processing mask for \""
                << view->get_name() << "\"..." << std::endl;
            mve::CameraInfo cam = view->get_camera();
            math::Matrix4f wtc;
            cam.fill_world_to_cam(*wtc);
            math::Matrix3f calib;
            cam.fill_calibration(*calib, mask->width(), mask->height());

            /* Iterate every point and check with mask. */
            for (std::size_t j = 0; j < verts.size(); ++j)
            {
                if (delete_list[j])
                    continue;
                math::Vec3f p = calib * wtc.mult(verts[j], 1.0f);
                p[0] = p[0] / p[2];
                p[1] = p[1] / p[2];
                if (p[0] < 0.0f || p[1] < 0.0f
                    || p[0] >= mask->width() || p[1] >= mask->height())
                    continue;
                int const ix = static_cast<int>(p[0]);
                int const iy = static_cast<int>(p[1]);
                if (mask->at(ix, iy, 0) == 0)
                {
                    delete_list[j] = true;
                    num_filtered += 1;
                }
            }
            view->cache_cleanup();
        }
        pset->delete_vertices(delete_list);
        std::cout << "Filtered a total of " << num_filtered
            << " points." << std::endl;
    }

    /* Write mesh to disc. */
    std::cout << "Writing final point set ("
        << verts.size() << " points)..." << std::endl;
    if (util::string::right(conf.outmesh, 4) == ".ply")
    {
        mve::geom::SavePLYOptions opts;
        opts.write_vertex_normals = conf.with_normals;
        opts.write_vertex_values = conf.with_scale;
        opts.write_vertex_confidences = conf.with_conf;
        mve::geom::save_ply_mesh(pset, conf.outmesh, opts);
    }
    else
    {
        mve::geom::save_mesh(pset, conf.outmesh);
    }
    if (conf.output_correspondence && conf.aabb.empty() && conf.mask.empty())
        save_correspondence_data(corr_data, conf);

    return EXIT_SUCCESS;
}
