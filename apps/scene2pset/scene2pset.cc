#include <fstream>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cerrno>

#include "util/arguments.h"
#include "util/tokenizer.h"

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
    bool poisson;
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

void
write_poisson_mesh (mve::TriangleMesh::ConstPtr mesh, std::string const& fname)
{
    bool binary = false;
    if (util::string::right(fname, 6) == ".bnpts")
        binary = true;

    std::ofstream out(fname.c_str());
    if (!out.good())
        throw std::runtime_error(std::strerror(errno));

    mve::TriangleMesh::VertexList const& verts(mesh->get_vertices());
    mve::TriangleMesh::NormalList const& vnorm(mesh->get_vertex_normals());

    if (verts.size() != vnorm.size())
        throw std::runtime_error("Poisson mesh without normals");

    for (std::size_t i = 0; i < verts.size(); ++i)
    {
        math::Vec3f v(verts[i]);
        math::Vec3f n(vnorm[i]);

        if (binary)
        {
            out.write((char const*)*v, sizeof(float) * 3);
            out.write((char const*)*n, sizeof(float) * 3);
        }
        else
        {
            out << v[0] << " " << v[1] << " " << v[2] << " ";
            out << n[0] << " " << n[1] << " " << n[2] << std::endl;
        }
    }

    out.close();
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
    args.add_option('n', "with-normals", false, "Write points with normals");
    args.add_option('p', "poisson", false, "Write points in Poission format");
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
    for (util::ArgResult const* arg = args.next_result();
        arg != 0; arg = args.next_result())
    {
        if (arg->opt == 0)
            continue;

        switch (arg->opt->sopt)
        {
            case 'p': conf.poisson = true; break;
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
        std::cout << "Got AABB: (" << aabbmin << ") / ("
            << aabbmax << ")" << std::endl;
    }
    std::cout << "Using depthmap: " << conf.dmname << " and color image: "
          << conf.image << std::endl;

    mve::TriangleMesh::Ptr pset(mve::TriangleMesh::create());
    mve::TriangleMesh::VertexList& verts(pset->get_vertices());
    mve::TriangleMesh::NormalList& vnorm(pset->get_vertex_normals());
    mve::TriangleMesh::ColorList& vcolor(pset->get_vertex_colors());

#if 0
    mve::TriangleMesh::Ptr mesh1 = mve::geom::load_ply_mesh("/gris/gris-f/home/sfuhrman/offmodels/stanford/bunny/meshes/depthmap-1.ply");
    mve::TriangleMesh::Ptr mesh2 = mve::geom::load_ply_mesh("/gris/gris-f/home/sfuhrman/offmodels/stanford/bunny/meshes/depthmap-5.ply");
    mesh1->ensure_normals();
    mesh2->ensure_normals();

    verts.insert(verts.end(), mesh1->get_vertices().begin(), mesh1->get_vertices().end());
    vnorm.insert(vnorm.end(), mesh1->get_vertex_normals().begin(), mesh1->get_vertex_normals().end());
    verts.insert(verts.end(), mesh2->get_vertices().begin(), mesh2->get_vertices().end());
    vnorm.insert(vnorm.end(), mesh2->get_vertex_normals().begin(), mesh2->get_vertex_normals().end());
#endif

#if 1
    /* Load scene. */
    mve::Scene::Ptr scene(mve::Scene::create());
    scene->load_scene(conf.scenedir);

    /* Iterate over views and get points. */
    mve::Scene::ViewList& views(scene->get_views());
    for (std::size_t i = 0; i < views.size(); ++i)
    {
        mve::View::Ptr view = views[i];
        if (!view.get())
            continue;

        std::size_t view_id = view->get_id();
        if (!conf.ids.empty() && std::find(conf.ids.begin(), conf.ids.end(), view_id) == conf.ids.end())
            continue;

        mve::FloatImage::Ptr dm = view->get_float_image(conf.dmname);
        if (!dm.get())
            continue;

        mve::ByteImage::Ptr ci;
        if (!conf.image.empty())
            ci = view->get_byte_image(conf.image);

        std::cout << "Processing view \"" << view->get_name()
            << "\"" << (ci.get() ? " (with colors)" : "")
            << "..." << std::endl;

        /* Triangulate depth map. */
        mve::CameraInfo const& cam = view->get_camera();
        mve::TriangleMesh::Ptr mesh;
        mesh = mve::geom::depthmap_triangulate(dm, ci, cam);

        mve::TriangleMesh::VertexList const& mverts(mesh->get_vertices());
        mve::TriangleMesh::NormalList const& mnorms(mesh->get_vertex_normals());
        mve::TriangleMesh::ColorList const& mvcol(mesh->get_vertex_colors());


        /* Add vertices and optional colors and normals to mesh. */

        /* Fast if no bounding box given */
        if (conf.aabb.empty())
        {
          verts.insert(verts.end(), mverts.begin(), mverts.end());
          if (!mvcol.empty())
                vcolor.insert(vcolor.end(), mvcol.begin(), mvcol.end());
          if (conf.with_normals)
            {
              mesh->ensure_normals();
              vnorm.insert(vnorm.end(), mnorms.begin(), mnorms.end());
            }
        }
        else
        {
          /* We need to iterate over all points */
          if (conf.with_normals)
            {
              mesh->ensure_normals();
            }
          for (unsigned int i =0; i < mverts.size(); i++){
            math::Vec3f pt = mverts[i];
            if (pt[0] < aabbmin[0] || pt[0] > aabbmax[0])
              continue;
            if (pt[1] < aabbmin[1] || pt[1] > aabbmax[1])
              continue;
            if (pt[2] < aabbmin[2] || pt[2] > aabbmax[2])
              continue;

            verts.push_back(pt);
            if (!mvcol.empty())
            {
              vcolor.push_back(mvcol[i]);
            }
            if (conf.with_normals)
            {
              vnorm.push_back(mnorms[i]);
            }
          }
        }
    }
#endif

    std::cout << "Writing final point set..." << std::endl;
    std::cout << "  Points: " << verts.size() << std::endl;
    std::cout << "  Normals: " << vnorm.size() << std::endl;
    std::cout << "  Colors: " << vcolor.size() << std::endl;

    /* Write mesh to disc. */
    if (conf.poisson)
    {
        write_poisson_mesh(pset, conf.outmesh);
    }
    else
    {
        mve::geom::save_ply_mesh(pset, conf.outmesh,
            true, true, conf.with_normals);
    }

    return 1;
}
