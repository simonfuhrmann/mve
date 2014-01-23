#include <cstring>
#include <cerrno>
#include <iostream>
#include <algorithm>
#include <fstream>

#include "math/matrixtools.h"
#include "math/geometry.h"

#include "util/filesystem.h"
#include "util/arguments.h"
#include "util/timer.h"

#include "mve/meshtools.h"
#include "mve/marchingcubes.h"
#include "mve/marchingtets.h"

#include "libdmfusion/octree.h"
#include "libdmfusion/octreeaccessor.h"

#define TETLIBRARY
#include "tetgen.h"

typedef dmf::Octree::VoxelMap VoxelMap;
typedef VoxelMap::iterator VoxelIter;
typedef VoxelMap::const_iterator VoxelConstIter;
typedef std::vector<VoxelIter> VoxelIterList;
typedef std::vector<dmf::VoxelIndex> VoxelIndexList;

/* ---------------------------------------------------------------- */

struct AppSettings
{
    std::string octreefile;
    std::string outfile;
    int force_level;
    float conf_thres;
    float boost_thres;
    bool color_levels;
};

/* ---------------------------------------------------------------- */

void
apply_mc (dmf::Octree& octree, AppSettings const& conf)
{
    std::cout << "Applying marching cubes..." << std::endl;
    dmf::OctreeMCAccessor accessor(octree);
    accessor.at_level = conf.force_level;
    accessor.min_weight = conf.conf_thres;
    mve::TriangleMesh::Ptr mesh = mve::geom::marching_cubes(accessor);
    mve::geom::save_mesh(mesh, conf.outfile);
}

/* ---------------------------------------------------------------- */

#if 0 // Abandoned code snipped to evaluate gradient direction of a tet
math::Vec3f
sdf_gradient (math::Vec3f const& a, math::Vec3f const& b, math::Vec3f const& c,
    math::Vec3f const& d, float sa, float sb, float sc, float sd)
{
    math::Matrix4f mat(1.0f);
    for (int i = 0; i < 3; ++i)
    {
        mat[i * 4 + 0] = a[i];
        mat[i * 4 + 1] = b[i];
        mat[i * 4 + 2] = c[i];
        mat[i * 4 + 3] = d[i];
    }
    mat = math::matrix_inverse(mat);
    return math::Vec3f(sa * mat[0] + sb * mat[4] + sc * mat[8] + sd * mat[12],
        sa * mat[1] + sb * mat[5] + sc * mat[9] + sd * mat[13],
        sa * mat[2] + sb * mat[6] + sc * mat[10] + sd * mat[14]);
}
#endif

/* ---------------------------------------------------------------- */

math::Vec4f
get_level_color (char min_level, char max_level, char level)
{

    math::Vec4f col1(0.0f, 1.0f, 0.0f);
    math::Vec4f col2(1.0f, 1.0f, 0.0f);
    math::Vec4f col3(1.0f, 0.0f, 0.0f);

    if (level < min_level)
        return col1;
    if (level > max_level)
        return col3;

    float lev = (float)(level - min_level);
    float range = (float)(max_level - min_level);
    float pos = (2.0f * lev) / range;

    if (pos < 1.0f)
        return col1 * (1.0f - pos) + col2 * pos;
    else
        return col2 * (2.0f - pos) + col3 * (pos - 1.0f);

#if 0
    float l = (float)level * 1.5f;
    float r = math::algo::clamp(0.2f * l - 2.0f);
    float g = math::algo::clamp(2.0f - 0.2f * l);
    float b = math::algo::clamp(2.0f - std::abs(2.0f - 0.2f * l));

    return math::Vec4f(r, g, b, 1.0f);
#endif
}

/* ---------------------------------------------------------------- */

int
main (int argc, char** argv)
{
    /* Setup argument parser. */
    util::Arguments args;
    args.set_description("Loads the octree from disc, boosts voxel values "
        "by interpolating parent voxels and removes unconfident voxels. "
        "A single level mesh using Marching Cubes can directly be produced. "
        "Otherwise, a tetrahedral mesh is constructed from the SDF values "
        "and written to disc.");
    args.add_option('b', "boost-thres", true, "Voxel boosting threshold [3.0]");
    args.add_option('c', "color-levels", false, "Colors samples according to their level");
    args.add_option('t', "conf-thres", true, "Confidence threshold [0.0]");
    args.add_option('f', "force-level", true, "Extract surface at single level [0]");
    args.set_exit_on_error(true);
    args.set_nonopt_maxnum(2);
    args.set_nonopt_minnum(2);
    args.set_helptext_indent(25);
    args.set_usage("Usage: dmfextract [ OPTIONS ] IN_OCTREE OUT_TETMESH");
    args.parse(argc, argv);

    /* Setup defaults. */
    AppSettings conf;
    conf.octreefile = args.get_nth_nonopt(0);
    conf.outfile = args.get_nth_nonopt(1);
    conf.force_level = 0;
    conf.conf_thres = 0.0f;
    conf.boost_thres = 3.0f;
    conf.color_levels = false;

    /* Process arguments. */
    for (util::ArgResult const* i = args.next_result();
        i != 0; i = args.next_result())
    {
        if (i->opt == 0)
            continue;

        switch (i->opt->sopt)
        {
            case 'f': conf.force_level = i->get_arg<int>(); break;
            case 't': conf.conf_thres = i->get_arg<float>(); break;
            case 'b': conf.boost_thres = i->get_arg<float>(); break;
            case 'c': conf.color_levels = true;
            default: break;
        }
    }

    /* Check output file name extension. */
    if (util::string::right(conf.outfile, 4) != ".ply")
    {
        std::cerr << "Error: Output file type must be .ply" << std::endl;
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
    std::size_t octload_time(0);
    std::size_t boosting_time(0);
    std::size_t remove_time(0);
    std::size_t delaunay_time(0);

    /* Load octree into memory. */
    dmf::Octree octree;
    octree.load_octree(conf.octreefile);
    octload_time = timer.get_elapsed();
    std::cout << "Loading octree took " << octload_time << "ms." << std::endl;

    if (octree.get_voxels().empty())
    {
        std::cerr << "Loaded octree is empty. Exiting." << std::endl;
        return 1;
    }

    /* Fall back to MC if there's only one level in the octree. */
    int min_level = octree.get_voxels().begin()->first.level;
    int max_level = octree.get_voxels().rbegin()->first.level;
    if (min_level == max_level)
    {
        std::cout << "Notice: Falling back to MC surface extraction at level "
            << min_level << ". Output is a surface!" << std::endl;
        conf.force_level = min_level;
    }

    /* Voxel boosting (only for multi-level octree). */
    timer.reset();
    if (conf.boost_thres >= 0.0f && !conf.force_level)
    {
        std::cout << "Boosting octree values with threshold "
            << conf.boost_thres << "..." << std::endl;
        octree.boost_voxels(conf.boost_thres);
        boosting_time = timer.get_elapsed();
    }

    /* Remove unconfident voxels. */
    timer.reset();
    std::cout << "Removing unconfident and twin voxels..." << std::endl;
    if (conf.conf_thres >= 0.0f)
    {
        std::size_t removed = octree.remove_unconfident(conf.conf_thres);
        std::cout << "Removed " << removed << " unconfident voxels." << std::endl;
    }

    /* Remember one representant for coinciding voxels. */
    if (!conf.force_level)
    {
        std::size_t removed = octree.remove_twins();
        std::cout << "Removed " << removed << " duplicated voxels." << std::endl;
    }
    remove_time = timer.get_elapsed();
    std::cout << "Removing voxels took " << remove_time << "ms." << std::endl;

    /* Handle single level extraction request with MC. */
    if (conf.force_level)
    {
        apply_mc(octree, conf);
        return 0;
    }

    /*
     * Create a tetrahedral mesh using Delaunay triangulation, which creates
     * a convex complex. Remove tets that spawn between unconnected parts.
     * Finally, write tet mesh to file.
     */

    /* Prepare tetrahedral mesh data structure. */
    mve::TriangleMesh::Ptr tetmesh(mve::TriangleMesh::create());
    mve::TriangleMesh::FaceList& tets(tetmesh->get_faces());
    mve::TriangleMesh::VertexList& verts(tetmesh->get_vertices());
    mve::TriangleMesh::ColorList& colors(tetmesh->get_vertex_colors());
    mve::TriangleMesh::ConfidenceList& sdf(tetmesh->get_vertex_confidences());
    std::vector<dmf::VoxelIndex> vidx;
    math::Vec3f aabb_min = octree.aabb_min();
    math::Vec3f aabb_max = octree.aabb_max();

    VoxelMap const& vmap(octree.get_voxels());
    verts.reserve(vmap.size());
    colors.reserve(vmap.size());
    sdf.reserve(vmap.size());
    vidx.reserve(vmap.size());

    bool use_colors = false;
    for (VoxelConstIter i = vmap.begin(); i != vmap.end(); ++i)
    {
        vidx.push_back(i->first);
        verts.push_back(i->first.get_pos(aabb_min, aabb_max));
        sdf.push_back(i->second.dist); // Store dist in confidence
        if (conf.color_levels)
        {
            use_colors = true;
            colors.push_back(get_level_color(8, 11, i->first.level));
        }
        else
        {
            colors.push_back(i->second.color);
            if (i->second.color[3] > 0.0f)
                use_colors = true;
        }
    }

    if (!use_colors)
        colors.clear();

    /* Cleanup octree. */
    octree.clear();

    /* Pass data to tetgen to build the tetmesh. */
    std::cout << "Starting tetrahedralization..." << std::endl;
    {
        tetgenio in, out;
        in.initialize();
        in.firstnumber = 0;
        in.numberofpoints = verts.size();

        /* Copy vertices to TetGen storage. */
        in.pointlist = new float[in.numberofpoints * 3];
        std::copy(*verts[0], *verts[0] + verts.size() * 3, in.pointlist);

        timer.reset();
        char nothing[1] = { '\0' };
        tetrahedralize(nothing, &in, &out);
        delaunay_time = timer.get_elapsed();

        std::cout << "Tetrahedralization took " << delaunay_time << " ms." << std::endl;

        tets.reserve(out.numberoftetrahedra * 4);
        for (int i = 0; i < out.numberoftetrahedra; ++i)
        {
            int vid[4];
            for (int j = 0; j < 4; ++j)
                vid[j] = out.tetrahedronlist[(i << 2) + j];
            std::swap(vid[0], vid[1]);

            /* Build unique tetrahedra index. */
            int tetconfig = 0;
            for (int j = 0; j < 4; ++j)
                if (sdf[vid[j]] < 0.0f)
                    tetconfig |= (1 << j);

            /* Skip tets without ISO crossing. */
            if (tetconfig == 0x0 || tetconfig == 0xf)
                continue;

            /* Find voxel indices and calc per-tet normal. */
            dmf::VoxelIndex vis[4];
            for (int j = 0; j < 4; ++j)
                vis[j] = vidx[vid[j]];

#if 1
            /* Remove tets between unconnected parts. */
            if (!vis[0].is_neighbor(vis[1])
                || !vis[1].is_neighbor(vis[2])
                || !vis[2].is_neighbor(vis[0])
                || !vis[0].is_neighbor(vis[3])
                || !vis[1].is_neighbor(vis[3])
                || !vis[2].is_neighbor(vis[3]))
                continue;
#endif

            for (int j = 0; j < 4; ++j)
                tets.push_back(vid[j]);
        }
    }


    /* Write output tetmesh. */
    {
        //mve::geom::SavePLYOptions ply_opts;
        //ply_opts.format_binary = true;
        //ply_opts.write_vertex_colors = use_colors;
        //ply_opts.write_vertex_confidences = true;
        //ply_opts.verts_per_simplex = 4;
        //mve::geom::save_ply_mesh(tetmesh, conf.outfile, ply_opts);
        mve::geom::save_mesh(tetmesh, conf.outfile);
    }

    std::cout << "Timings (in milli seconds):" << std::endl
        << "  Loading octree from file: " << octload_time << std::endl
        << "  Boosting octree voxel values: " << boosting_time << std::endl
        << "  Removing voxels from octree: " << remove_time << std::endl
        << "  Building tetrahefral mesh: " << delaunay_time << std::endl;

    std::ofstream log("dmfextract.log", std::ios::app);
    log << std::endl << "CWD: " << util::fs::get_cwd_string() << std::endl;
    log << "Call:";
    for (int i = 0; i < argc; ++i)
        log << " " << argv[i];
    log << std::endl;
    log << "Timings (in milli seconds):" << std::endl
        << "  Loading octree from file: " << octload_time << std::endl
        << "  Boosting octree voxel values: " << boosting_time << std::endl
        << "  Removing voxels from octree: " << remove_time << std::endl
        << "  Building tetrahefral mesh: " << delaunay_time << std::endl;
    log.close();

    return 0;
}


/*
 * Abandoned code snippest involving CGAL.
 */
#define USE_CGAL_TETRAHEDRALIZATION 0
#if USE_CGAL_TETRAHEDRALIZATION
#   include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#   include <CGAL/Triangulation_vertex_base_with_info_3.h>
#   include <CGAL/Delaunay_triangulation_3.h>

    typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
    typedef CGAL::Triangulation_vertex_base_with_info_3<std::size_t, Kernel> VertexBase;
    typedef CGAL::Triangulation_data_structure_3<VertexBase> TDS3;
    typedef CGAL::Delaunay_triangulation_3<Kernel, TDS3> Triangulation;

    typedef Triangulation::Cell_handle    Cell_handle;
    typedef Triangulation::Vertex_handle  Vertex_handle;
    typedef Triangulation::Tetrahedron    Tetrahedron;
    typedef Triangulation::Point          Point;

    typedef Triangulation::Finite_vertices_iterator Vertex_iter;
    typedef Triangulation::Finite_cells_iterator    Cell_iter;
#endif

#if USE_CGAL_TETRAHEDRALIZATION

    /*
     * Insert vertices into triangulation.
     */
    std::cout << "Inserting points into triangulation..." << std::endl;
    timer.reset();
    Triangulation tetmesh;
    for (std::size_t i = 0; i < verts.size(); ++i)
    {
        math::Vec3f const& v(verts[i]);
        Vertex_handle vh = tetmesh.insert(Point(v[0], v[1], v[2]));
        vh->info() = i;
        if (i % 1000 == 0)
        {
            std::cout << "\r" << "Inserted " << i
                << " of " << verts.size() << " points..." << std::flush;
        }
    }
    std::cout << " done." << std::endl;
    delaunay_time = timer.get_elapsed();

    /*
     * Feed MT accessor with tets.
     */
    std::cout << "Preparing MT accessor..." << std::endl;
    dmf::TetmeshAccessor accessor;
    accessor.use_color = use_color;
    std::swap(verts, accessor.verts);
    std::swap(cnf, accessor.sdf_values);
    std::swap(color, accessor.colors);
    pset.reset();

    /* Add tets with ISO surface to MT accessor. */
    for (Cell_iter iter = tetmesh.finite_cells_begin();
        iter != tetmesh.finite_cells_end(); ++iter)
    {
        Cell_handle cell = iter;

        unsigned int vid[4] =
        {
            // Flipped vertex 0 and 1 to re-orient tets
            cell->vertex(1)->info(), cell->vertex(0)->info(),
            cell->vertex(2)->info(), cell->vertex(3)->info()
        };

        int tetconfig = 0;
        for (int i = 0; i < 4; ++i)
            if (accessor.sdf_values[vid[i]] < 0.0f)
                tetconfig |= (1 << i);

        if (tetconfig == 0x0 || tetconfig == 0xf)
            continue;

        for (int i = 0; i < 4; ++i)
            accessor.tets.push_back(vid[i]);

/*
        dmf::VoxelIndex vis[4] =
        {
            accessor.verts[vid[0]],
            accessor.verts[vid[1]],
            accessor.verts[vid[2]],
            accessor.verts[vid[3]]
        };

        if (!vis[0].is_neighbor(vis[1])
            || !vis[1].is_neighbor(vis[2])
            || !vis[2].is_neighbor(vis[0])
            || !vis[0].is_neighbor(vis[3])
            || !vis[1].is_neighbor(vis[3])
            || !vis[2].is_neighbor(vis[3]))
            continue;
*/

    }

#endif

