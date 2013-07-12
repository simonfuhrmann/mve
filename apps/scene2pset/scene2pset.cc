#include <fstream>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cerrno>

#include "util/arguments.h"
#include "util/tokenizer.h"
#include "mve/meshtools.h"
#include "mve/depthmap.h"
#include "mve/plyfile.h"
#include "mve/scene.h"
#include "mve/view.h"

struct AppSettings
{
    std::string scenedir;
    std::string outmesh;
    std::string dmname;
    std::string image;
    std::string aabb;
    bool with_normals;
    std::vector<std::size_t> ids;
};

void
parse_ids (std::string const& id_string, std::vector<std::size_t>& ids)
{
    ids.clear();
    if (id_string == "all")
        return;
    util::Tokenizer t;
    t.split(id_string, ',');
    for (std::size_t i = 0; i < t.size(); ++i)
        ids.push_back(util::string::convert<std::size_t>(t[i]));
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
        "Generates a pointset from selected views by projecting "
        "reconstructed depth values to the world coordinate system. "
        "By default, all views are used.");
    args.add_option('n', "with-normals", false, "Write points with normals (PLY only)");
    args.add_option('s', "with-scale", false, "Write points with scale information (PLY only)");
    args.add_option('d', "depthmap", true, "Name of depthmap to use [depthmap]");
    args.add_option('i', "image", true, "Name of color image to use [undistorted]");
    args.add_option('v', "views", true, "View IDs to use for reconstruction [all]");
    args.add_option('b', "bounding-box", true, "Six comma separated values used as AABB.");

    args.parse(argc, argv);

    /* Init default settings. */
    AppSettings conf;
    conf.scenedir = args.get_nth_nonopt(0);
    conf.outmesh = args.get_nth_nonopt(1);
    conf.with_normals = false;
    conf.poisson = false;
    conf.dmname = "depthmap";

    /* Scan arguments. */
    while (util::ArgResult const* arg = args.next_result())
    {
        if (arg->opt == NULL)
            continue;

        switch (arg->opt->sopt)
        {
            case 'n': conf.with_normals = true; break;
            case 'd': conf.dmname = arg->arg; break;
            case 'i': conf.image = arg->arg; break;
            case 'v': parse_ids(arg->arg, conf.ids); break;
            case 'b': conf.aabb = arg->arg; break;
            default: throw std::runtime_error("Unknown option");
        }
    }

    if (conf.poisson)
        conf.with_normals = true;

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

        mve::ByteImage::Ptr ci;
        if (!conf.image.empty())
            ci = view->get_byte_image(conf.image);

#pragma omp critical
        std::cout << "Processing view \"" << view->get_name()
            << "\"" << (ci.get() ? " (with colors)" : "")
            << "..." << std::endl;

        /* Triangulate depth map. */
        mve::CameraInfo const& cam = view->get_camera();
        mve::TriangleMesh::Ptr mesh;
        mesh = mve::geom::depthmap_triangulate(dm, ci, cam);
        if (conf.with_normals)
            mesh->ensure_normals();

        mve::TriangleMesh::VertexList const& mverts(mesh->get_vertices());
        mve::TriangleMesh::NormalList const& mnorms(mesh->get_vertex_normals());
        mve::TriangleMesh::ColorList const& mvcol(mesh->get_vertex_colors());

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
            }
        }
        else
        {
            /* Check every point if a bounding box is given.  */
            if (conf.with_normals)
                mesh->ensure_normals();

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
                }
            }
        }

        dm.reset();
        ci.reset();
        view->cache_cleanup();
    }

    std::cout << "Writing final point set..." << std::endl;
    std::cout << "  Points: " << verts.size() << std::endl;
    std::cout << "  Normals: " << vnorm.size() << std::endl;
    std::cout << "  Colors: " << vcolor.size() << std::endl;

    /* Write mesh to disc. */
    if (util::string::right(conf.outmesh, 4) == ".ply")
    {
        mve::geom::SavePLYOptions opts;
        opts.write_vertex_normals = conf.with_normals;
        opts.write_vertex_values = conf.write_scale;
        mve::geom::save_ply_mesh(pset, conf.outmesh, opts);
    }
    else
    {
        mve::geom::save_mesh(pset, conf.outmesh);
    }

    return 0;
}
