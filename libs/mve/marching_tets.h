/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef MVE_MARCHINGTETS_HEADER
#define MVE_MARCHINGTETS_HEADER

#include <map>

#include "math/vector.h"
#include "math/functions.h"
#include "mve/defines.h"
#include "mve/mesh.h"

MVE_NAMESPACE_BEGIN
MVE_GEOM_NAMESPACE_BEGIN

/**
 * This function polygonizes a SDF that is partitioned into tetrahedrons.
 * It requires an accessor that acts as iterator and provides information
 * about the SDF. This interface decouples the algorithm from the
 * underlying representation.
 *
 * The accessor must support the following operations:
 * - Iterating over the tets: bool accessor.next()
 * - Accessing the SDF values: accessor.sdf[4]
 * - Accessing the Tet vertex IDs: accessor.vid[4]
 * - Accessing the Tet vertex positions: accessor.pos[4][3]
 *
 * - NEW: Determining color support: bool accessor.has_colors()
 * - NEW: Accessing Tet vertex colors: accessor.color[4][3]
 *
 * The first call to next() must initialize the accessor.
 * next() must return false if there is no next element, true otherwise.
 * The accessor is supposed to iterate over valid tets only.
 */
template <typename T>
TriangleMesh::Ptr
marching_tetrahedra (T& accessor);

/* ------------------------- Lookup tables ------------------------ */

/**
 * Freudenthal cube partitioning, that subdivides the cube
 * into 6 tetrahera for continuous reconstruction. This can be used
 * as template when designing accessors for regular volumes.
 * See also: illustrations/cube_subdivisions.png
 */
extern int mt_freudenthal[6][4];

/**
 * Defines the 6-bit edge mask (for all 6 edges in a tet) for
 * all 16 tetrahedron configurations. See marching.cc.
 */
extern int mt_edge_table[16];

/**
 * Defines the triangle setup for the 16 tetrahedron configurations.
 * Three subsequent vertices form a triangle, max 2 triangles per tet.
 * The vertices are indexed by tet edge ID. See marching.cc.
 */
extern int mt_tri_table[16][7];

/**
 * Ordering in which edges in the tetrahedron are defined. First edge
 * between tet vertices 0 and 1, and so on. See marching.cc.
 */
extern int mt_edge_order[6][2];

/* ------------------------- Implementation ----------------------- */

template <typename T>
TriangleMesh::Ptr
marching_tetrahedra (T& accessor)
{
    TriangleMesh::Ptr ret(TriangleMesh::create());
    TriangleMesh::VertexList& verts(ret->get_vertices());
    TriangleMesh::FaceList& faces(ret->get_faces());
    TriangleMesh::ColorList& colors(ret->get_vertex_colors());

    typedef std::pair<std::size_t, std::size_t> Edge;
    typedef std::map<Edge, unsigned int> EdgeMap;
    typedef std::map<std::size_t, unsigned int> VertexMap;
    EdgeMap edge_map;
    VertexMap vert_map;

    while (accessor.next())
    {
        /* Build unique cube index. */
        int tetconfig = 0;
        for (int i = 0; i < 4; ++i)
            if (accessor.sdf[i] < 0.0f)
                tetconfig |= (1 << i);

        if (tetconfig == 0x0 || tetconfig == 0xf)
            continue;

        /* Get unique 6 bit edge configuration. */
        int const edgeconfig = mt_edge_table[tetconfig];

        /* Iterate of the ISO edges and provide vertex IDs. */
        unsigned int vid[6];
        for (int i = 0; i < 6; ++i)
        {
            if (!(edgeconfig & (1 << i)))
                continue;

            int const* ev = mt_edge_order[i];

            /* Try to find vertex ID for edge. */
            Edge edge(accessor.vid[ev[0]], accessor.vid[ev[1]]);
            if (edge.first > edge.second)
                std::swap(edge.first, edge.second);

            EdgeMap::iterator iter = edge_map.find(edge);
            if (iter != edge_map.end())
            {
                vid[i] = iter->second;
                continue;
            }

            /* Fetch distance values of the edge. */
            float d[2] = { accessor.sdf[ev[0]], accessor.sdf[ev[1]] };

            /* NEW: Vertex snapping to prevent null faces. */
            int snap = -1;
            if (d[0] == 0.0f)
                snap = ev[0];
            else if (d[1] == 0.0f)
                snap = ev[1];

            if (snap >= 0)
            {
                VertexMap::iterator iter = vert_map.find(accessor.vid[snap]);
                if (iter != vert_map.end())
                {
                    vid[i] = iter->second;
                    continue;
                }

                if (accessor.has_colors())
                    colors.push_back(math::Vec4f(accessor.color[snap], 1.0f));
                vid[i] = verts.size();
                verts.push_back(accessor.pos[snap]);
                vert_map.insert(std::make_pair(accessor.vid[snap], vid[i]));
                continue;
            }

            /* Interpolate vertex position. */
            float w[2] = { d[1] / (d[1] - d[0]), -d[0] / (d[1] - d[0]) };
            math::Vec3f x = math::interpolate
                (accessor.pos[ev[0]], accessor.pos[ev[1]], w[0], w[1]);

            /* Iterpolate color value on the edge. */
            if (accessor.has_colors())
            {
                math::Vec3f col = math::interpolate
                    (accessor.color[ev[0]], accessor.color[ev[1]], w[0], w[1]);
                colors.push_back(math::Vec4f(col, 1.0f));
            }

            vid[i] = verts.size();
            verts.push_back(x);
            edge_map.insert(std::make_pair(edge, vid[i]));
        }

        /* Generate triangles by connecting the vertex IDs. */
        for (int i = 0; mt_tri_table[tetconfig][i] != -1; i += 3)
        {
            unsigned int vids[3];
            for (int k = 0; k < 3; ++k)
                vids[k] = vid[mt_tri_table[tetconfig][i + k]];
            if (vids[0] != vids[1] && vids[1] != vids[2] && vids[2] != vids[0])
                for (int k = 0; k < 3; ++k)
                    faces.push_back(vids[k]);
        }
    }

    return ret;
}

MVE_GEOM_NAMESPACE_END
MVE_NAMESPACE_END

#endif /* MVE_MARCHINGTETS_HEADER */
