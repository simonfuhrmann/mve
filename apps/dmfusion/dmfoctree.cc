/*
 * Part of the depth map fusion (dmf) framework toolset.
 * Written by Simon Fuhrmann.
 *
 * Bounding Boxes
 * --------------
 *
 * Wall AABB: (-33.0, -15, -7.4) (-20, 7.6, 10)
 *      AABB2: -40.0 -12 -7.4 -22 7.6 10
 * Hanau AABB: (-3.0, -1.5, -0.6) (-1.0, 0.5, 1.7)
 * Notre Dame AABB: (-25, -10.5, -1.17) (-18, -0.13, 13.2)
 *            AABB2: -30 -10.5 -1.17 -18 -0.13 13.5
 * Memorial AABB: (-6.0f, -3.0f, -8.0f) (2.0f, 4.0f, 4.0f)
 * Fabian Wall AABB: -4 0 -0.5 -2 2 1
 *
 * Stanford Datasets Reconstruction
 * --------------------------------
 *
 * dmfoctree -r6 -f9 -x <PATH>/bunny/data/bun.conf <PATH>/bunny-R6.octree
 * dmfsurface -t0.04 -f9 <PATH>/bunny-R6.octree <PATH>/bunny-R6-T004.ply
 */

#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cstring>
#include <cerrno>

#include "util/filesystem.h"
#include "util/arguments.h"
#include "util/timer.h"
#include "util/tokenizer.h"

#include "mve/scene.h"
#include "mve/view.h"
#include "mve/meshtools.h"
#include "mve/depthmap.h"

#include "libdmfusion/stanford.h"
#include "libdmfusion/octree.h"

struct AppSettings
{
    std::string dataset;
    std::string outfile;
    std::string octree;
    std::string debug_dm;
    std::string depthmap;
    std::string image;
    std::string viewids;
    std::string aabb;
    std::size_t maxview;

    int border_dw;
    int border_peel;
    float ramp_factor;
    float sampling_rate;
    int force_level;
    bool no_expansion;
    int coarser_levels;
};

/* ---------------------------------------------------------------- */

void
insert_mesh (AppSettings const& conf, dmf::Octree& octree,
    mve::TriangleMesh::Ptr mesh, math::Vec3f const& campos)
{
    /* Peel triangles away from the boundary. */
    mve::geom::depthmap_mesh_peeling(mesh, conf.border_peel);

    /* Apply low confidences to mesh vertices at the boundary. */
    mve::geom::depthmap_mesh_confidences(mesh, conf.border_dw);

    /* If depth mesh debug mode is enabled, write depthmaps and exit. */
    if (!conf.debug_dm.empty())
    {
        static int cnt = 0;
        cnt += 1;
        std::stringstream ss;
        ss << conf.debug_dm << "/depthmap-" << cnt << ".ply";
        mesh->get_vertices().push_back(campos);
        if (mesh->get_vertex_colors().size() + 1 == mesh->get_vertices().size())
            mesh->get_vertex_colors().push_back(math::Vec4f(1, 0, 0, 1));
        mve::geom::save_mesh(mesh, ss.str());
        return;
    }

    /* Make sure we have normals. */
    mesh->ensure_normals(false, true);

    /* Finally insert. */
    octree.insert(mesh, campos);
}

/* ---------------------------------------------------------------- */

void
fuse_stanford (AppSettings const& conf, dmf::Octree& octree)
{
    dmf::StanfordDataset scene;
    scene.read_config(conf.dataset);

    util::ClockTimer timer;
    dmf::StanfordDataset::RangeImages const& set(scene.get_range_images());
    std::size_t num_views = 0;
    for (std::size_t i = 0; i <  set.size(); ++i)
    {
        dmf::StanfordRangeImage const& ri(set[i]);
        mve::TriangleMesh::Ptr mesh(scene.get_mesh(set[i]));

        std::cout << "Mesh filename: " << ri.filename << std::endl;
        std::cout << "Camera position: " << ri.campos << std::endl;
        std::cout << "Viewing direction: " << ri.viewdir << std::endl;

        /* For stanford data, we assume orthographic camera. */
        octree.set_orthographic_viewdir(ri.viewdir);

        std::cout << "Inserting DM " << i << " ("
            << ri.filename << ") into octree..." << std::endl;
        util::ClockTimer dmtimer;
        //insert_mesh(conf, octree, mesh, ri.campos);
        std::cout << "  took " << dmtimer.get_elapsed() << "ms." << std::endl;
        octree.get_memory_usage();

        num_views += 1;
        if (conf.maxview && num_views >= conf.maxview)
            break;
    }

    std::cout << "Done inserting " << num_views << " depthmaps, took "
        << timer.get_elapsed() << "ms." << std::endl;
}

/* ---------------------------------------------------------------- */

void
fuse_mve (AppSettings const& conf, dmf::Octree& octree)
{
    mve::Scene scene;
    scene.load_scene(conf.dataset);

    /* If requested, use given AABB. */
    if (!conf.aabb.empty())
    {
        util::Tokenizer tok;
        tok.split(conf.aabb, ',');
        if (tok.size() != 6)
        {
            std::cout << "Error: Invalid AABB given" << std::endl;
            std::exit(1);
        }

        math::Vec3f aabbmin, aabbmax;
        for (int i = 0; i < 3; ++i)
        {
            aabbmin[i] = util::string::convert<float>(tok[i]);
            aabbmax[i] = util::string::convert<float>(tok[i + 3]);
        }
        octree.set_forced_aabb(aabbmin, aabbmax);

        std::cout << "Got AABB: (" << aabbmin << ") / ("
            << aabbmax << ")" << std::endl;
    }

    /* Available views. */
    mve::Scene::ViewList const& views = scene.get_views();

    /* Enumerate all views to insert... */
    std::vector<std::size_t> viewids;
    if (!conf.viewids.empty())
    {
        /* View IDs are given on the command line. */
        util::Tokenizer t;
        t.split(conf.viewids, ',');
        for (std::size_t i = 0; i < t.size(); ++i)
        {
            std::size_t pos = t[i].find_first_of('-');
            if (pos != std::string::npos)
            {
                int first = util::string::convert<int>(t[i].substr(0, pos));
                int last = util::string::convert<int>(t[i].substr(pos + 1));
                //std::cout << "First: " << first << ", last: " << last << std::endl;
                for (int j = first; j <= last; ++j)
                    viewids.push_back(j);
            }
            else
            {
                viewids.push_back(util::string::convert<std::size_t>(t[i]));
            }
        }
    }
    else
    {
        /* View IDs are not given, use all. */
        for (std::size_t i = 0; i < views.size(); ++i)
            viewids.push_back(i);
    }

    std::cout << "Got a total of " << viewids.size()
        << " views to fuse." << std::endl;

    /* Insert all enumerated views. */
    util::ClockTimer timer;
    std::size_t num_views = 0;
    for (std::size_t i = 0; i < viewids.size(); ++i)
    {
        std::size_t vid = viewids[i];
        mve::View::Ptr view = views[vid];
        if (!view.get())
            continue;

        mve::FloatImage::Ptr dm = view->get_float_image(conf.depthmap);
        mve::ByteImage::Ptr ci = view->get_byte_image(conf.image);
        mve::CameraInfo const& cam = view->get_camera();
        if (!dm.get() || !ci.get() || cam.flen == 0.0f){
            std::cout << "Could not load depthmap " << conf.depthmap
                      << " or color image " << conf.image << " for view "<< i << std::endl;
            continue;
        }

        /* Triangulate depth map. */
        mve::TriangleMesh::Ptr mesh
            = mve::geom::depthmap_triangulate(dm, ci, cam);

        math::Vec3f campos;
        cam.fill_camera_pos(*campos);

        std::cout << "Inserting view " << vid << " into octree..." << std::endl;
        util::ClockTimer dmtimer;
        insert_mesh(conf, octree, mesh, campos);
        std::cout << "  took " << dmtimer.get_elapsed() << "ms." << std::endl;
        octree.get_memory_usage();

        num_views += 1;
        if (conf.maxview && num_views >= conf.maxview)
            break;
    }

    std::cout << "Done inserting " << num_views << " depthmaps, took "
        << timer.get_elapsed() << "ms." << std::endl;
}

/* ---------------------------------------------------------------- */

int main (int argc, char** argv)
{
    /* Setup argument parser. */
    util::Arguments args;
    args.add_option('r', "ramp-size", true, "Ramp size factor [4.0]");
    args.add_option('s', "sampling-rate", true, "Triangle sampling rate [1.0]");
    args.add_option('f', "force-level", true, "Forces all triangles to fixed level [0]");
    args.add_option('m', "maxviews", true, "Maximum number of views to insert [0]");
    args.add_option('v', "view-ids", true, "Specify view IDs to insert into octree");
    args.add_option('d', "depthmap", true, "Depth map name (for MVE datasets) [depth-L1]");
    args.add_option('i', "image", true, "Color image name (for MVE datasets) [undist-L1]");
    args.add_option('o', "octree", true, "Load and fuse into existing octree");
    args.add_option('b', "bounding-box", true, "Six comma separated values used as AABB.");
    args.add_option('x', "no-expansion", false, "Disallows octree expansion");
    args.add_option('c', "coarser-levels", true, "Inserts into number of coarser levels [2]");
    args.add_option('w', "border-dw", true, "Boundary down-weighting (iterations) [3]");
    args.add_option('p', "border-peel", true, "Peel triangles at mesh boundary [0]");
    args.add_option('y', "debug-dm", true, "Writes depth meshes to given directory");
    args.set_usage(argv[0], "[ OPTIONS ] IN_DATASET OUT_OCTREE");
    args.set_description("Fuses a set of depth meshes into an multi-scale "
        "SDF implemented using an octree. Input can either be a set of MVE "
        "depth maps or a set of meshes given in Stanford format. Note that "
        "a camera position is required for each depth mesh. Construction of "
        "the octree can be controlled in several ways.");
    args.set_exit_on_error(true);
    args.set_nonopt_maxnum(2);
    args.set_nonopt_minnum(2);
    args.set_helptext_indent(25);
    args.parse(argc, argv);

    /* Setup defaults. */
    AppSettings conf;
    conf.depthmap = "depth-L0";
    conf.image = "undistorted";
    conf.maxview = 0;
    conf.border_dw = 3;
    conf.border_peel = 0;
    conf.ramp_factor = 4.0f;
    conf.sampling_rate = 1.0f;
    conf.force_level = 0;
    conf.coarser_levels = 2;
    conf.no_expansion = false;

    /* Process arguments. */
    int nonopt_iter = 0;
    for (util::ArgResult const* i = args.next_result();
        i != 0; i = args.next_result())
    {
        if (i->opt == 0)
        {
            if (nonopt_iter == 0)
                conf.dataset = i->arg;
            else if (nonopt_iter == 1)
                conf.outfile = i->arg;
            nonopt_iter += 1;
            continue;
        }

        switch (i->opt->sopt)
        {
            case 'd': conf.depthmap = i->arg; break;
            case 'i': conf.image = i->arg; break;
            case 'w': conf.border_dw = i->get_arg<int>(); break;
            case 'p': conf.border_peel = i->get_arg<int>(); break;
            case 'r': conf.ramp_factor = i->get_arg<float>(); break;
            case 's': conf.sampling_rate = i->get_arg<float>(); break;
            case 'f': conf.force_level = i->get_arg<int>(); break;
            case 'm': conf.maxview = i->get_arg<std::size_t>(); break;
            case 'v': conf.viewids = i->arg; break;
            case 'x': conf.no_expansion = true; break;
            case 'o': conf.octree = i->arg; break;
            case 'y': conf.debug_dm = i->arg; break;
            case 'b': conf.aabb = i->arg; break;
            case 'c': conf.coarser_levels = i->get_arg<int>(); break;
            default: break;
        }
    }

    if (conf.outfile.empty() || conf.dataset.empty())
    {
        args.generate_helptext(std::cerr);
        return 1;
    }

    /* Test-open output file. */
    {
        std::ofstream out(conf.outfile.c_str(), std::ios::app);
        if (!out.good())
        {
            std::cerr << "Error opening output file: "
                << ::strerror(errno) << std::endl;
            return 1;
        }
        out.close();
    }

    util::ClockTimer timer;
    std::size_t build_octree_time(0);
    std::size_t save_octree_time(0);

    /* Init octree. */
    dmf::Octree octree;
    octree.set_ramp_factor(conf.ramp_factor);
    octree.set_sampling_rate(conf.sampling_rate);
    octree.set_forced_level(conf.force_level);
    octree.set_allow_expansion(!conf.force_level && !conf.no_expansion);
    octree.set_coarser_levels(conf.coarser_levels);

    /* Load octree from file if requested. */
    if (!conf.octree.empty())
    {
        timer.reset();
        octree.load_octree(conf.octree);
        std::cout << "Loading octree took "
            << timer.get_elapsed() << "ms." << std::endl;
    }

    /* Check which dataset is specified, MVE or Stanford format. */
    timer.reset();
    if (util::fs::file_exists(conf.dataset.c_str()))
    {
        /* A file was specified, it's a Stanford config file. */
        try
        {
            fuse_stanford(conf, octree);
        }
        catch (std::exception& e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    }
    else if (util::fs::dir_exists(conf.dataset.c_str()))
    {
        /* A directory was specified, it's an MVE dataset. */
        try
        {
            fuse_mve(conf, octree);
        }
        catch (std::exception& e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    }
    else
    {
        std::cerr << "Error: Unrecognized dataset!" << std::endl;
        return 1;
    }
    build_octree_time = timer.get_elapsed();

    /* Check if octree has voxels. */
    if (octree.get_voxels().empty())
    {
        std::cerr << "Error: Empty octree, exiting!" << std::endl;
        return 1;
    }

    /* Save octree to file. */
    std::cout << "Saving octree (" << octree.get_voxels().size()
        << " voxels) to file..." << std::flush;
    timer.reset();
    octree.save_octree(conf.outfile);
    std::cout << " done. Took " << timer.get_elapsed() << "ms." << std::endl;
    save_octree_time = timer.get_elapsed();

    /*
     * Some timing results.
     */
    std::cout << "Timings:" << std::endl
        << "  Building octree: " << build_octree_time << std::endl
        << "  Saving octree to file: " << save_octree_time << std::endl;

    std::ofstream log("dmfoctree.log", std::ios::app);
    log << std::endl << "CWD: " << util::fs::get_cwd_string() << std::endl;
    log << "Call:";
    for (int i = 0; i < argc; ++i)
        log << " " << argv[i];
    log << std::endl;
    log << "Timings:" << std::endl
        << "  Building octree: " << build_octree_time << std::endl
        << "  Saving octree to file: " << save_octree_time << std::endl;
    log.close();

    return 0;
}
