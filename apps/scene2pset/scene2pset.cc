#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <string>
#include <cstring>
#include <cerrno>

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
    std::string dmname;
    std::string image;
    std::string mask;
    std::string aabb;
    bool with_normals;
    bool with_scale;
    bool with_conf;
    bool poisson_normals;
    float min_valid_fraction;
    std::vector<int> ids;
};

void
poisson_scale_normals (mve::TriangleMesh::ConfidenceList const& confs,
    mve::TriangleMesh::NormalList* normals)
{
    if (confs.size() != normals->size())
        throw std::invalid_argument("Invalid confidences or normals");
    for (std::size_t i = 0; i < confs.size(); ++i)
        normals->at(i) *= confs[i];
}

int
main (int argc, char** argv)
{
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
    args.add_option('d', "depthmap", true, "Name of depthmap to use [depth-L0]");
    args.add_option('i', "image", true, "Name of color image to use [undistorted]");
    args.add_option('n', "with-normals", false, "Write points with normals (PLY only)");
    args.add_option('s', "with-scale", false, "Write points with scale values (PLY only)");
    args.add_option('c', "with-conf", false, "Write points with confidence (PLY only)");
    args.add_option('m', "mask", true, "Name of mask/silhouette image to clip 3D points []");
    args.add_option('v', "views", true, "View IDs to use for reconstruction [all]");
    args.add_option('b', "bounding-box", true, "Six comma separated values used as AABB.");
    args.add_option('f', "min-fraction", true, "Minimum fraction of valid depth values [0.0]");
    args.add_option('p', "poisson-normals", false, "Scale normals according to confidence");
    args.parse(argc, argv);

    /* Init default settings. */
    AppSettings conf;
    conf.scenedir = args.get_nth_nonopt(0);
    conf.outmesh = args.get_nth_nonopt(1);
    conf.dmname = "depth-L0";
    conf.image = "undistorted";
    conf.with_normals = false;
    conf.with_scale = false;
    conf.with_conf = false;
    conf.poisson_normals = false;
    conf.min_valid_fraction = 0.0f;

    /* Scan arguments. */
    while (util::ArgResult const* arg = args.next_result())
    {
        if (arg->opt == NULL)
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
    {
        util::Tokenizer tok;
        tok.split(conf.aabb, ',');
        if (tok.size() != 6)
        {
            std::cout << "Error: Invalid AABB given" << std::endl;
            std::exit(1);
        }

        for (int i = 0; i < 3; ++i)
        {
            aabbmin[i] = util::string::convert<float>(tok[i]);
            aabbmax[i] = util::string::convert<float>(tok[i + 3]);
        }
        std::cout << "Using AABB: (" << aabbmin << ") / ("
            << aabbmax << ")" << std::endl;
    }
    std::cout << "Using depthmap: " << conf.dmname << " and color image: "
          << conf.image << std::endl;

    /* Prepare output mesh. */
    mve::TriangleMesh::Ptr pset(mve::TriangleMesh::create());
    mve::TriangleMesh::VertexList& verts(pset->get_vertices());
    mve::TriangleMesh::NormalList& vnorm(pset->get_vertex_normals());
    mve::TriangleMesh::ColorList& vcolor(pset->get_vertex_colors());
    mve::TriangleMesh::ValueList& vvalues(pset->get_vertex_values());
    mve::TriangleMesh::ConfidenceList& vconfs(pset->get_vertex_confidences());

    /* Load scene. */
    mve::Scene::Ptr scene(mve::Scene::create());
    scene->load_scene(conf.scenedir);

    /* Iterate over views and get points. */
    mve::Scene::ViewList& views(scene->get_views());
#pragma omp parallel for schedule(dynamic)
    for (std::size_t i = 0; i < views.size(); ++i)
    {
        mve::View::Ptr view = views[i];
        if (view == NULL)
            continue;

        std::size_t view_id = view->get_id();
        if (!conf.ids.empty() && std::find(conf.ids.begin(),
            conf.ids.end(), view_id) == conf.ids.end())
            continue;

        mve::FloatImage::Ptr dm = view->get_float_image(conf.dmname);
        if (dm == NULL)
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
            << "\"" << (ci != NULL ? " (with colors)" : "")
            << "..." << std::endl;

        /* Triangulate depth map. */
        mve::CameraInfo const& cam = view->get_camera();
        mve::TriangleMesh::Ptr mesh;
        mesh = mve::geom::depthmap_triangulate(dm, ci, cam);
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
            mve::VertexInfoList::Ptr vinfo = mve::VertexInfoList::create(mesh);
            for (std::size_t j = 0; j < vinfo->size(); ++j)
            {
                mve::MeshVertexInfo const& vinf = vinfo->at(j);
                for (std::size_t k = 0; k < vinf.verts.size(); ++k)
                    mvscale[j] += (mverts[j] - mverts[vinf.verts[k]]).norm();
                mvscale[j] /= static_cast<float>(vinf.verts.size());
                mvscale[j] *= 2.5f;  /* MVS patch size is usually 5x5. */
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
            }
        }
        else
        {
            /* Check every point if a bounding box is given.  */
            for (std::size_t i = 0; i < mverts.size(); ++i)
            {
                math::Vec3f const& pt = mverts[i];
                if (pt[0] < aabbmin[0] || pt[0] > aabbmax[0]
                    || pt[1] < aabbmin[1] || pt[1] > aabbmax[1]
                    || pt[2] < aabbmin[2] || pt[2] > aabbmax[2])
                    continue;

#pragma omp critical
                {
                    verts.push_back(pt);
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
            if (view == NULL || view->get_camera().flen == 0.0f)
                continue;
            mve::ByteImage::Ptr mask = view->get_byte_image(conf.mask);
            if (mask == NULL)
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

    return 0;
}
