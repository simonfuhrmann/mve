#include <cstring>
#include <cerrno>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <limits>

#include "util/file_system.h"
#include "util/arguments.h"
#include "util/timer.h"

#include "mve/mesh_io.h"
#include "mve/mesh_tools.h"
#include "mve/marching_tets.h"

#include "libdmfusion/tetmeshaccessor.h"

struct AppSettings
{
    std::string inputfile;
    std::string outputfile;
    float no_mt_optimize;
};

/* ---------------------------------------------------------------- */

void
tetmesh_vertexsnap (mve::TriangleMesh::Ptr tetmesh)
{
    mve::TriangleMesh::VertexList& verts(tetmesh->get_vertices());
    mve::TriangleMesh::FaceList& tets(tetmesh->get_faces());
    mve::TriangleMesh::ConfidenceList& sdf(tetmesh->get_vertex_confidences());

    /* Iterate over all edges in the tets. */
    std::size_t snap_cnt = 0;
    for (std::size_t i = 0; i < tets.size(); i += 4)
    {
        for (int j = 0; j < 6; ++j)
        {
            int off1 = mve::geom::mt_edge_order[j][0];
            int off2 = mve::geom::mt_edge_order[j][1];
            float sdf1 = sdf[tets[i + off1]];
            float sdf2 = sdf[tets[i + off2]];

            /* Ignore edge if one of the vertices has zero SDF. */
            if (sdf1 == 0.0f || sdf2 == 0.0f)
                continue;

            int edgeconfig = (sdf1 < 0.0f) | (sdf2 < 0.0f) << 1;

            switch (edgeconfig)
            {
                /* If sdf1 < sdf2, swap and handle the opposite case. */
                case 1:
                    std::swap(off1, off2);
                    std::swap(sdf1, sdf2);

                /* Handle sdf2 < sdf1. */
                case 2:
                    if (sdf1 / (sdf1 - sdf2) < 0.2f)
                    {
                        float w1 = sdf1 / (sdf1 - sdf2);
                        float w2 = 1.0f - w1;
                        sdf[tets[i + off1]] = 0.0f;
                        verts[tets[i + off1]] = math::algo::interpolate
                            (verts[tets[i + off2]], verts[tets[i + off1]],
                            w1, w2);
                        snap_cnt += 1;
                    }
                    break;

                /* Ignore edges without zero crossing. */
                case 0: case 3: default:
                    break;
            }
        }
    }
    std::cout << "Snapped " << snap_cnt << " vertices!" << std::endl;
}

/* ---------------------------------------------------------------- */

int
main (int argc, char** argv)
{
    /* Setup argument parser. */
    util::Arguments args;
    args.set_description("Loads a tetmesh with SDF and optional color "
        "values from file (PLY format) and applies Marching Tetrahedra "
        "in order to extract the ISO surface. Simplification of the "
        "resulting surface is done by snapping vertices in the tetrahedral "
        "complex to the ISO surface.");
    args.add_option('n', "no-optimize", false, "Don't optimize tetrahedral complex for MT");
    args.set_exit_on_error(true);
    args.set_nonopt_maxnum(2);
    args.set_nonopt_minnum(2);
    args.set_helptext_indent(25);
    args.set_usage("Usage: dmfsurface [ OPTIONS ] IN_TETMESH OUT_TRIMESH");
    args.parse(argc, argv);

    /* Setup defaults. */
    AppSettings conf;
    conf.inputfile = args.get_nth_nonopt(0);
    conf.outputfile = args.get_nth_nonopt(1);
    conf.no_mt_optimize = false;

    /* Process arguments. */
    for (util::ArgResult const* i = args.next_result();
        i != 0; i = args.next_result())
    {
        if (i->opt == 0)
            continue;

        switch (i->opt->sopt)
        {
            case 'n': conf.no_mt_optimize = true;
            default: break;
        }
    }

    /* Test-open output file. */
    {
        std::ofstream out(conf.outputfile.c_str(), std::ios::app);
        if (!out.good())
        {
            std::cerr << "Error opening output file: "
                << ::strerror(errno) << std::endl;
            return 1;
        }
        out.close();
    }

    util::ClockTimer timer;
    std::size_t meshload_time(0);
    std::size_t simplify_time(0);
    std::size_t meshgen_time(0);

    /* Load point set into memory. */
    mve::TriangleMesh::Ptr tetmesh;
    try
    {
        std::cout << "Loading tetmesh from file..." << std::endl;
        tetmesh = mve::geom::load_mesh(conf.inputfile);
        meshload_time = timer.get_elapsed();
    }
    catch (std::exception& e)
    {
        std::cerr << "Error loading tet mesh: " << e.what() << std::endl;
        return 1;
    }
    std::cout << "Done loading mesh, took " << meshload_time << "ms, "
         << "consumes " << (tetmesh->get_byte_size() / (1 << 20))
         << " MB memory." << std::endl;

    mve::TriangleMesh::VertexList& verts(tetmesh->get_vertices());
    mve::TriangleMesh::FaceList& tets(tetmesh->get_faces());
    mve::TriangleMesh::ConfidenceList& sdf(tetmesh->get_vertex_confidences());
    mve::TriangleMesh::ColorList& color(tetmesh->get_vertex_colors());
    bool use_color = (color.size() == verts.size());

    if (verts.empty())
    {
        std::cout << "Error: Tetmesh set contains no points!" << std::endl;
        return 1;
    }

    if (sdf.size() != verts.size())
    {
        std::cout << "Error: No distance values given!" << std::endl;
        return 1;
    }

    if (tets.empty())
    {
        std::cout << "Error: Tetmesh contains no simplices!" << std::endl;
        return 1;
    }

    /* Snap vertices near ISO surface to surface. */
    if (!conf.no_mt_optimize)
    {
        std::cout << "Optimizing tetrahedral complex..." << std::endl;
        timer.reset();
        tetmesh_vertexsnap(tetmesh);
        std::cout << "Snapping took " << timer.get_elapsed() << "ms." << std::endl;
    }

    /* Feed MT accessor with tets. */
    std::cout << "Preparing MT accessor..." << std::endl;
    dmf::TetmeshAccessor accessor;
    accessor.use_color = use_color;
    std::swap(verts, accessor.verts);
    std::swap(sdf, accessor.sdf_values);
    std::swap(color, accessor.colors);
    std::swap(tets, accessor.tets);
    tetmesh.reset();

    std::cout << "Accessor has " << accessor.sdf_values.size() << " SDF values"
        << ", and " << accessor.verts.size() << " vertices"
        << ", and " << accessor.tets.size() << " tets" << std::endl;

    /*
     * Triangulating mesh with Marching Tetrahedra (MT).
     */
    std::cout << "Triangulating with Tet-MT..." << std::endl;
    timer.reset();
    mve::TriangleMesh::Ptr surface = mve::geom::marching_tetrahedra(accessor);
    mve::geom::save_mesh(surface, conf.outputfile);
    meshgen_time = timer.get_elapsed();

    /*
     * Some timing results.
     */
    std::cout << "Timings:" << std::endl
        << "  Loading point set from file: " << meshload_time << std::endl
        << "  Tetrahedral simplification: " << simplify_time << std::endl
        << "  Surface extraction with MT: " << meshgen_time << std::endl;

    /* Logging. */
    std::ofstream log("dmfsurface.log", std::ios::app);
    log << std::endl << "CWD: " << util::fs::get_cwd_string() << std::endl;
    log << "Call:";
    for (int i = 0; i < argc; ++i)
        log << " " << argv[i];
    log << std::endl;
    log << "Timings:" << std::endl
        << "  Loading point set from file: " << meshload_time << std::endl
        << "  Tetrahedral simplification: " << simplify_time << std::endl
        << "  Surface extraction with MT: " << meshgen_time << std::endl;
    log.close();

    return 0;
}
