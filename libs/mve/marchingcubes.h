/*
 * Implementation of the marching cubes algorithm.
 * Written by Simon Fuhrmann.
 */

#ifndef MVE_MARCHINGCUBES_HEADER
#define MVE_MARCHINGCUBES_HEADER

#include <map>

#include "math/vector.h"
#include "math/algo.h"
#include "mve/defines.h"
#include "mve/trianglemesh.h"
#include "mve/image.h"

MVE_NAMESPACE_BEGIN
MVE_GEOM_NAMESPACE_BEGIN

/**
 * This function polygonizes a SDF that is partitioned into cubes.
 * It requires an accessor that acts as iterator and provides information
 * about the SDF. This interface decouples the algorithm from the
 * underlying representation.
 *
 * The accessor must support the following operations:
 * - Iterating over the cubes: bool accessor.next()
 * - Accessing the SDF values: accessor.sdf[8]
 * - Accessing the cube vertex IDs: accessor.vid[8]
 * - Accessint the cube vertex positions: accessor.pos[8][3]
 * - Determining color support: bool accessor.has_colors()
 * - Accessing Tet vertex colors: accessor.color[4][3]
 *
 * The first call to next() must initialize the accessor.
 * next() must return false if there is no next element, true otherwise.
 *
 * The ordering of the voxels and the edges of a cube are given in the
 * following. The ordering of the edges is not important for using the
 * algorithm, but the voxels need to be provided in the right order.
 *
 *      4------5        +-- 4--+        +------+
 *     /|     /|       7|     5|       /|     /|        y
 *    7-+----6 |      +-+ 6--+ |      +-+----+ |        |
 *    | |    | |      | |    | |      | 8    | 9        |
 *    | 0----+-1      | +--0-+-+      | +----+-+        +------ x
 *    |/     |/       |3     |1      11/    10/        /
 *    3------2        +--2---+        +------+        z
 *   Vertex Order    Edge Order 1    Edge Order 2
 */
template <typename T>
TriangleMesh::Ptr
marching_cubes (T& accessor);

/**
 * This function creates an image visualizing the SDF on a single slice
 * in the volume. The method requires the same accessor as for the
 * marching cubes algorithm. The maximum distance (absolute value) has
 * to be provided, after this distance the colors stay black.
 */
//template <typename T>
//ByteImage::Ptr
//mc_volume_slicer (T& accessor, int axis, std::size_t slice);

MVE_GEOM_NAMESPACE_END
MVE_NAMESPACE_END

/* ------------------------- Lookup tables ------------------------ */

MVE_NAMESPACE_BEGIN
MVE_GEOM_NAMESPACE_BEGIN

/**
 * Defines the 12-bit edge mask, each bit corresponding to one of 12 edges,
 * that contain the surface for all 256 cube configurations.
 */
extern int mc_edge_table[256];

/**
 * Defines the triangle setup for the 256 cube configurations.
 * For each cube configuration, three subsequent vertices form a triangle.
 * The vertices are indexed by cube edge ID.
 */
extern int mc_tri_table[256][16];

/**
 * The ordering in which edges of the cube are defined.
 * The first edge is between cube vertices 0 and 1, and so on.
 */
extern int mc_edge_order[12][2];

/* ------------------------- Implementation ----------------------- */

template <typename T>
TriangleMesh::Ptr
marching_cubes (T& accessor)
{
    TriangleMesh::Ptr ret(TriangleMesh::create());
    TriangleMesh::VertexList& verts(ret->get_vertices());
    TriangleMesh::FaceList& faces(ret->get_faces());
    TriangleMesh::ColorList& colors(ret->get_vertex_colors());

    typedef std::pair<std::size_t, std::size_t> Edge;
    typedef std::map<Edge, std::size_t> EdgeMap;
    EdgeMap vert_ids;

    while (accessor.next())
    {
        /* Builds a unique cube index given the SDF values at the cube voxels. */
        int cubeconfig = 0;
        for (int i = 0; i < 8; ++i)
            if (accessor.sdf[i] < 0.0f)
                cubeconfig |= (1 << i);

        /* Try early bail out. */
        if (cubeconfig == 0x00 || cubeconfig == 0xff)
            continue;

        /* Get unique 12 bit edge configuration. */
        int edgeconfig = mc_edge_table[cubeconfig];

        /* Iterate over ISO edges and provide vertex IDs. */
        std::size_t vid[12];
        for (int i = 0; i < 12; ++i)
        {
            if (!(edgeconfig & (1 << i)))
                continue;

            int const* ev = mc_edge_order[i];

            /* Try to find vertex ID for edge. */
            Edge edge(accessor.vid[ev[0]], accessor.vid[ev[1]]);
            if (edge.first > edge.second)
                std::swap(edge.first, edge.second);

            EdgeMap::iterator iter = vert_ids.find(edge);
            if (iter != vert_ids.end())
            {
                vid[i] = iter->second;
                continue;
            }

            /* Create new vertex on the edge. */
            float d[2] = { accessor.sdf[ev[0]], accessor.sdf[ev[1]] };
            float w[2] = { d[1] / (d[1] - d[0]), -d[0] / (d[1] - d[0]) };
            math::Vec3f x = math::algo::interpolate
                (accessor.pos[ev[0]], accessor.pos[ev[1]], w[0], w[1]);

            if (accessor.has_colors())
            {
                math::Vec3f col = math::algo::interpolate
                    (accessor.color[ev[0]], accessor.color[ev[1]], w[0], w[1]);
                colors.push_back(math::Vec4f(col, 1.0f));
            }

            vid[i] = verts.size();
            vert_ids.insert(std::make_pair(edge, verts.size()));
            verts.push_back(x);
        }

        /* Generate triangles by connecting the vertex IDs. */
        for (int j = 0; mc_tri_table[cubeconfig][j] != -1; j += 3)
            for (int k = 0; k < 3; ++k)
                faces.push_back(vid[mc_tri_table[cubeconfig][j + k]]);
    }

    return ret;
}

MVE_GEOM_NAMESPACE_END
MVE_NAMESPACE_END

#endif /* MVE_MARCHINGCUBES_HEADER */
