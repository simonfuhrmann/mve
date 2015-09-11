/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 *
 * The code assumes an ordering of cube vertices and cube edges as follows:
 *
 *      2------3        +---3--+        +------+
 *     /|     /|       5|    11|       /|     /|        y
 *    6-+----7 |      +-+ 9--+ |      +-+----+ |        |
 *    | |    | |      | |    | |      | 4    | 1        |
 *    | 0----+-1      | +--0-+-+      | +----+-+        +------ x
 *    |/     |/       |8     |2       7/    10/        /
 *    4------5        +--6---+        +------+        z
 *   Vertex Order    Edge Order 1    Edge Order 2
 */

#include <iostream>
#include <bitset>

#include "util/timer.h"
#include "fssr/octree.h"
#include "fssr/iso_surface.h"
#include "fssr/triangulation.h"

#define ISO_VALUE 0.0f
#define CUBE_CORNERS 8
#define CUBE_EDGES 12
#define CUBE_FACES 6

FSSR_NAMESPACE_BEGIN

/** Helper types and functions. */
namespace
{
    /** Cube face directions. */
    enum CubeFace
    {
        POSITIVE_X = 0,
        NEGATIVE_X = 1,
        POSITIVE_Y = 2,
        NEGATIVE_Y = 3,
        POSITIVE_Z = 4,
        NEGATIVE_Z = 5
    };

    CubeFace const face_opposite[6] =
    {
        NEGATIVE_X, POSITIVE_X,
        NEGATIVE_Y, POSITIVE_Y,
        NEGATIVE_Z, POSITIVE_Z
    };

    /** The four corner IDs around a cube face. */
    int const face_corners[6][4] =
    {
        { 1, 3, 5, 7 }, /* Positive X. */
        { 0, 2, 4, 6 }, /* Negative X. */
        { 2, 3, 6, 7 }, /* Positive Y. */
        { 0, 1, 4, 5 }, /* Negative Y. */
        { 4, 5, 6, 7 }, /* Positive Z. */
        { 0, 1, 2, 3 }, /* Negative Z. */
    };

    bool const is_edge_on_face[12][6] =
    {
        /* PosX  NegX   PosY   NegY   PosZ   NegZ */
        { false, false, false, true , false, true  }, /* Edge 0. */
        { true , false, false, false, false, true  }, /* Edge 1. */
        { true , false, false, true , false, false }, /* Edge 2. */
        { false, false, true , false, false, true  }, /* Edge 3. */
        { false, true , false, false, false, true  }, /* Edge 4. */
        { false, true , true , false, false, false }, /* Edge 5. */
        { false, false, false, true , true , false }, /* Edge 6. */
        { false, true , false, false, true , false }, /* Edge 7. */
        { false, true , false, true , false, false }, /* Edge 8. */
        { false, false, true , false, true , false }, /* Edge 9. */
        { true , false, false, false, true , false }, /* Edge 10. */
        { true , false, true , false, false, false }, /* Edge 11. */
    };

    /** The directions of neighboring nodes around an edge. */
    CubeFace const edge_neighbors[12][2] =
    {
        { NEGATIVE_Y, NEGATIVE_Z }, /* Edge 0. */
        { POSITIVE_X, NEGATIVE_Z }, /* Edge 1. */
        { POSITIVE_X, NEGATIVE_Y }, /* Edge 2. */
        { POSITIVE_Y, NEGATIVE_Z }, /* Edge 3. */
        { NEGATIVE_X, NEGATIVE_Z }, /* Edge 4. */
        { NEGATIVE_X, POSITIVE_Y }, /* Edge 5. */
        { NEGATIVE_Y, POSITIVE_Z }, /* Edge 6. */
        { NEGATIVE_X, POSITIVE_Z }, /* Edge 7. */
        { NEGATIVE_X, NEGATIVE_Y }, /* Edge 8. */
        { POSITIVE_Y, POSITIVE_Z }, /* Edge 9. */
        { POSITIVE_X, POSITIVE_Z }, /* Edge 10. */
        { POSITIVE_X, POSITIVE_Y }  /* Edge 11. */
    };

    /** Voxel corners and children IDs of an edge. */
    int const edge_corners[12][2] =
    {
        { 0, 1 }, /* Edge 0. */
        { 1, 3 }, /* Edge 1. */
        { 1, 5 }, /* Edge 2. */
        { 2, 3 }, /* Edge 3. */
        { 0, 2 }, /* Edge 4. */
        { 2, 6 }, /* Edge 5. */
        { 4, 5 }, /* Edge 6. */
        { 4, 6 }, /* Edge 7. */
        { 0, 4 }, /* Edge 8. */
        { 6, 7 }, /* Edge 9. */
        { 5, 7 }, /* Edge 10. */
        { 3, 7 }, /* Edge 11. */
    };

    /** Same edge ID but seen from adjacent nodes. */
    int const edge_reflections[12][3] =
    {
        {  3,  6,  9 }, /* Edge 0. */
        {  4, 10,  7 }, /* Edge 1. */
        {  8, 11,  5 }, /* Edge 2. */
        {  0,  9,  6 }, /* Edge 3. */
        {  1,  7, 10 }, /* Edge 4. */
        { 11,  8,  2 }, /* Edge 5. */
        {  9,  0,  3 }, /* Edge 6. */
        { 10,  4,  1 }, /* Edge 7. */
        {  2,  5, 11 }, /* Edge 8. */
        {  6,  3,  0 }, /* Edge 9. */
        {  7,  1,  4 }, /* Edge 10. */
        {  5,  2,  8 }, /* Edge 11. */
    };

    /** Specifies for each MC case which edges to connect to a polygon. */
    int const mc_polygons[256][17] =
    {
        { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  0,  4,  8,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  0,  2,  1,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  4,  8,  2,  1,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  3,  5,  4,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  8,  0,  3,  5,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  0,  2,  1,  0,  4,  3,  5,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  2,  1,  3,  5,  8,  2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  1, 11,  3,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  1, 11,  3,  1,  0,  4,  8,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        { 11,  3,  0,  2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        { 11,  3,  4,  8,  2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  5,  4,  1, 11,  5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  8,  0,  1, 11,  5,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  5,  4,  0,  2, 11,  5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  2, 11,  5,  8,  2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  8,  7,  6,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  7,  6,  0,  4,  7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  2,  1,  0,  2,  6,  8,  7,  6, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  7,  6,  2,  1,  4,  7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  4,  3,  5,  4,  8,  7,  6,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  3,  5,  7,  6,  0,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  1,  0,  2,  1,  4,  3,  5,  4,  6,  8,  7,  6, -1, -1, -1, -1, -1 },
        {  3,  5,  7,  6,  2,  1,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  1, 11,  3,  1,  6,  8,  7,  6, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  7,  6,  0,  4,  7,  3,  1, 11,  3, -1, -1, -1, -1, -1, -1, -1, -1 },
        { 11,  3,  0,  2, 11,  6,  8,  7,  6, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  6,  2, 11,  3,  4,  7,  6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  1, 11,  5,  4,  1,  8,  7,  6,  8, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  1, 11,  5,  7,  6,  0,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  6,  8,  7,  6,  0,  2, 11,  5,  4,  0, -1, -1, -1, -1, -1, -1, -1 },
        {  7,  6,  2, 11,  5,  7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  2,  6, 10,  2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  0,  4,  8,  0,  2,  6, 10,  2, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  6, 10,  1,  0,  6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  4,  8,  6, 10,  1,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  2,  6, 10,  2,  4,  3,  5,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  3,  5,  8,  0,  3,  2,  6, 10,  2, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  6, 10,  1,  0,  6,  4,  3,  5,  4, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  6, 10,  1,  3,  5,  8,  6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  2,  6, 10,  2,  1, 11,  3,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  8,  0,  4,  8,  2,  6, 10,  2,  3,  1, 11,  3, -1, -1, -1, -1, -1 },
        {  6, 10, 11,  3,  0,  6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  6, 10, 11,  3,  4,  8,  6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  5,  4,  1, 11,  5, 10,  2,  6, 10, -1, -1, -1, -1, -1, -1, -1, -1 },
        { 10,  2,  6, 10,  1, 11,  5,  8,  0,  1, -1, -1, -1, -1, -1, -1, -1 },
        {  4,  0,  6, 10, 11,  5,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  6, 10, 11,  5,  8,  6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        { 10,  2,  8,  7, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        { 10,  2,  0,  4,  7, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  1,  0,  8,  7, 10,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  1,  4,  7, 10,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        { 10,  2,  8,  7, 10,  5,  4,  3,  5, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  7, 10,  2,  0,  3,  5,  7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  3,  5,  4,  3,  1,  0,  8,  7, 10,  1, -1, -1, -1, -1, -1, -1, -1 },
        {  3,  5,  7, 10,  1,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  8,  7, 10,  2,  8,  1, 11,  3,  1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  3,  1, 11,  3,  0,  4,  7, 10,  2,  0, -1, -1, -1, -1, -1, -1, -1 },
        { 11,  3,  0,  8,  7, 10, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        { 11,  3,  4,  7, 10, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  1, 11,  5,  4,  1, 10,  2,  8,  7, 10, -1, -1, -1, -1, -1, -1, -1 },
        { 11,  5,  7, 10,  2,  0,  1, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  7, 10, 11,  5,  4,  0,  8,  7, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  7, 10, 11,  5,  7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  5,  9,  7,  5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  8,  0,  4,  8,  7,  5,  9,  7, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  0,  2,  1,  0,  7,  5,  9,  7, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  2,  1,  4,  8,  2,  7,  5,  9,  7, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  4,  3,  9,  7,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  9,  7,  8,  0,  3,  9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  9,  7,  4,  3,  9,  1,  0,  2,  1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  1,  3,  9,  7,  8,  2,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  3,  1, 11,  3,  5,  9,  7,  5, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  0,  4,  8,  0,  3,  1, 11,  3,  7,  5,  9,  7, -1, -1, -1, -1, -1 },
        {  0,  2, 11,  3,  0,  5,  9,  7,  5, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  7,  5,  9,  7,  4,  8,  2, 11,  3,  4, -1, -1, -1, -1, -1, -1, -1 },
        {  1, 11,  9,  7,  4,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  9,  7,  8,  0,  1, 11,  9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  7,  4,  0,  2, 11,  9,  7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  9,  7,  8,  2, 11,  9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  6,  8,  5,  9,  6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  0,  4,  5,  9,  6,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  5,  9,  6,  8,  5,  0,  2,  1,  0, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  9,  6,  2,  1,  4,  5,  9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  6,  8,  4,  3,  9,  6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  0,  3,  9,  6,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  2,  1,  0,  2,  6,  8,  4,  3,  9,  6, -1, -1, -1, -1, -1, -1, -1 },
        {  2,  1,  3,  9,  6,  2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  6,  8,  5,  9,  6, 11,  3,  1, 11, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  1, 11,  3,  1,  0,  4,  5,  9,  6,  0, -1, -1, -1, -1, -1, -1, -1 },
        {  9,  6,  8,  5,  9,  2, 11,  3,  0,  2, -1, -1, -1, -1, -1, -1, -1 },
        {  9,  6,  2, 11,  3,  4,  5,  9, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  4,  1, 11,  9,  6,  8,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  1, 11,  9,  6,  0,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  2, 11,  9,  6,  8,  4,  0,  2, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  2, 11,  9,  6,  2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        { 10,  2,  6, 10,  9,  7,  5,  9, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  2,  6, 10,  2,  8,  0,  4,  8,  9,  7,  5,  9, -1, -1, -1, -1, -1 },
        {  1,  0,  6, 10,  1,  9,  7,  5,  9, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  9,  7,  5,  9,  6, 10,  1,  4,  8,  6, -1, -1, -1, -1, -1, -1, -1 },
        {  4,  3,  9,  7,  4,  6, 10,  2,  6, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  2,  6, 10,  2,  8,  0,  3,  9,  7,  8, -1, -1, -1, -1, -1, -1, -1 },
        {  7,  4,  3,  9,  7,  0,  6, 10,  1,  0, -1, -1, -1, -1, -1, -1, -1 },
        { 10,  1,  3,  9,  7,  8,  6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  1, 11,  3,  1, 10,  2,  6, 10,  5,  9,  7,  5, -1, -1, -1, -1, -1 },
        {  9,  7,  5,  9,  2,  6, 10,  2,  0,  4,  8,  0,  3,  1, 11,  3, -1 },
        {  5,  9,  7,  5, 11,  3,  0,  6, 10, 11, -1, -1, -1, -1, -1, -1, -1 },
        {  3,  4,  8,  6, 10, 11,  3,  7,  5,  9,  7, -1, -1, -1, -1, -1, -1 },
        {  2,  6, 10,  2,  1, 11,  9,  7,  4,  1, -1, -1, -1, -1, -1, -1, -1 },
        {  7,  8,  0,  1, 11,  9,  7,  2,  6, 10,  2, -1, -1, -1, -1, -1, -1 },
        {  7,  4,  0,  6, 10, 11,  9,  7, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  9,  7,  8,  6, 10, 11,  9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  5,  9, 10,  2,  8,  5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  5,  9, 10,  2,  0,  4,  5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  0,  8,  5,  9, 10,  1,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  5,  9, 10,  1,  4,  5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  2,  8,  4,  3,  9, 10,  2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        { 10,  2,  0,  3,  9, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  3,  9, 10,  1,  0,  8,  4,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        { 10,  1,  3,  9, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  1, 11,  3,  1, 10,  2,  8,  5,  9, 10, -1, -1, -1, -1, -1, -1, -1 },
        {  9, 10,  2,  0,  4,  5,  9,  1, 11,  3,  1, -1, -1, -1, -1, -1, -1 },
        {  3,  0,  8,  5,  9, 10, 11,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  5,  9, 10, 11,  3,  4,  5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  2,  8,  4,  1, 11,  9, 10,  2, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        { 10,  2,  0,  1, 11,  9, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  0,  8,  4,  0, 11,  9, 10, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        { 10, 11,  9, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        { 10,  9, 11, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  0,  4,  8,  0, 11, 10,  9, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  1,  0,  2,  1, 11, 10,  9, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  4,  8,  2,  1,  4, 11, 10,  9, 11, -1, -1, -1, -1, -1, -1, -1, -1 },
        { 11, 10,  9, 11,  3,  5,  4,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  8,  0,  3,  5,  8,  9, 11, 10,  9, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  2,  1,  0,  2, 11, 10,  9, 11,  4,  3,  5,  4, -1, -1, -1, -1, -1 },
        { 10,  9, 11, 10,  2,  1,  3,  5,  8,  2, -1, -1, -1, -1, -1, -1, -1 },
        {  3,  1, 10,  9,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        { 10,  9,  3,  1, 10,  0,  4,  8,  0, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  0,  2, 10,  9,  3,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  4,  8,  2, 10,  9,  3,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        { 10,  9,  5,  4,  1, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  5,  8,  0,  1, 10,  9,  5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        { 10,  9,  5,  4,  0,  2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        { 10,  9,  5,  8,  2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  6,  8,  7,  6, 10,  9, 11, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  0,  4,  7,  6,  0, 10,  9, 11, 10, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  0,  2,  1,  0,  6,  8,  7,  6, 11, 10,  9, 11, -1, -1, -1, -1, -1 },
        { 11, 10,  9, 11,  2,  1,  4,  7,  6,  2, -1, -1, -1, -1, -1, -1, -1 },
        {  8,  7,  6,  8,  5,  4,  3,  5, 10,  9, 11, 10, -1, -1, -1, -1, -1 },
        { 11, 10,  9, 11,  3,  5,  7,  6,  0,  3, -1, -1, -1, -1, -1, -1, -1 },
        {  0,  2,  1,  0,  3,  5,  4,  3,  6,  8,  7,  6, 11, 10,  9, 11, -1 },
        {  6,  2,  1,  3,  5,  7,  6, 11, 10,  9, 11, -1, -1, -1, -1, -1, -1 },
        {  3,  1, 10,  9,  3,  7,  6,  8,  7, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  0,  4,  7,  6,  0,  3,  1, 10,  9,  3, -1, -1, -1, -1, -1, -1, -1 },
        {  8,  7,  6,  8,  0,  2, 10,  9,  3,  0, -1, -1, -1, -1, -1, -1, -1 },
        {  9,  3,  4,  7,  6,  2, 10,  9, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  6,  8,  7,  6, 10,  9,  5,  4,  1, 10, -1, -1, -1, -1, -1, -1, -1 },
        {  6,  0,  1, 10,  9,  5,  7,  6, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  4,  0,  2, 10,  9,  5,  4,  6,  8,  7,  6, -1, -1, -1, -1, -1, -1 },
        {  7,  6,  2, 10,  9,  5,  7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  9, 11,  2,  6,  9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  9, 11,  2,  6,  9,  8,  0,  4,  8, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  9, 11,  1,  0,  6,  9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        { 11,  1,  4,  8,  6,  9, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  2,  6,  9, 11,  2,  3,  5,  4,  3, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  2,  6,  9, 11,  2,  8,  0,  3,  5,  8, -1, -1, -1, -1, -1, -1, -1 },
        {  4,  3,  5,  4,  1,  0,  6,  9, 11,  1, -1, -1, -1, -1, -1, -1, -1 },
        {  5,  8,  6,  9, 11,  1,  3,  5, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  3,  1,  2,  6,  9,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  8,  0,  4,  8,  2,  6,  9,  3,  1,  2, -1, -1, -1, -1, -1, -1, -1 },
        {  0,  6,  9,  3,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  4,  8,  6,  9,  3,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  2,  6,  9,  5,  4,  1,  2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  6,  9,  5,  8,  0,  1,  2,  6, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  5,  4,  0,  6,  9,  5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  5,  8,  6,  9,  5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  8,  7,  9, 11,  2,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  0,  4,  7,  9, 11,  2,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  8,  7,  9, 11,  1,  0,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  9, 11,  1,  4,  7,  9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  4,  3,  5,  4,  8,  7,  9, 11,  2,  8, -1, -1, -1, -1, -1, -1, -1 },
        { 11,  2,  0,  3,  5,  7,  9, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        { 11,  1,  0,  8,  7,  9, 11,  4,  3,  5,  4, -1, -1, -1, -1, -1, -1 },
        {  9, 11,  1,  3,  5,  7,  9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  9,  3,  1,  2,  8,  7,  9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  4,  7,  9,  3,  1,  2,  0,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  8,  7,  9,  3,  0,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  9,  3,  4,  7,  9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  4,  1,  2,  8,  7,  9,  5,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  0,  1,  2,  0,  7,  9,  5,  7, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  5,  4,  0,  8,  7,  9,  5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  5,  7,  9,  5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        { 11, 10,  7,  5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        { 11, 10,  7,  5, 11,  4,  8,  0,  4, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  7,  5, 11, 10,  7,  2,  1,  0,  2, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  5, 11, 10,  7,  5,  1,  4,  8,  2,  1, -1, -1, -1, -1, -1, -1, -1 },
        {  4,  3, 11, 10,  7,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  0,  3, 11, 10,  7,  8,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  0,  2,  1,  0,  4,  3, 11, 10,  7,  4, -1, -1, -1, -1, -1, -1, -1 },
        { 10,  7,  8,  2,  1,  3, 11, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  7,  5,  3,  1, 10,  7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  0,  4,  8,  0,  3,  1, 10,  7,  5,  3, -1, -1, -1, -1, -1, -1, -1 },
        {  2, 10,  7,  5,  3,  0,  2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  8,  2, 10,  7,  5,  3,  4,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  1, 10,  7,  4,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  8,  0,  1, 10,  7,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  0,  2, 10,  7,  4,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  8,  2, 10,  7,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        { 11, 10,  6,  8,  5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  6,  0,  4,  5, 11, 10,  6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  1,  0,  2,  1, 11, 10,  6,  8,  5, 11, -1, -1, -1, -1, -1, -1, -1 },
        {  1,  4,  5, 11, 10,  6,  2,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        { 11, 10,  6,  8,  4,  3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        { 11, 10,  6,  0,  3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        { 10,  6,  8,  4,  3, 11, 10,  0,  2,  1,  0, -1, -1, -1, -1, -1, -1 },
        { 11, 10,  6,  2,  1,  3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  1, 10,  6,  8,  5,  3,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  1, 10,  6,  0,  4,  5,  3,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  8,  5,  3,  0,  2, 10,  6,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  2, 10,  6,  2,  4,  5,  3,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  6,  8,  4,  1, 10,  6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  1, 10,  6,  0,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  6,  8,  4,  0,  2, 10,  6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  2, 10,  6,  2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  2,  6,  7,  5, 11,  2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  0,  4,  8,  0,  2,  6,  7,  5, 11,  2, -1, -1, -1, -1, -1, -1, -1 },
        {  5, 11,  1,  0,  6,  7,  5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  5, 11,  1,  4,  8,  6,  7,  5, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        { 11,  2,  6,  7,  4,  3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  0,  3, 11,  2,  6,  7,  8,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  0,  6,  7,  4,  3, 11,  1,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  1,  3, 11,  1,  6,  7,  8,  6, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  7,  5,  3,  1,  2,  6,  7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  5,  3,  1,  2,  6,  7,  5,  0,  4,  8,  0, -1, -1, -1, -1, -1, -1 },
        {  7,  5,  3,  0,  6,  7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  7,  5,  3,  4,  8,  6,  7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  2,  6,  7,  4,  1,  2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  8,  0,  1,  2,  6,  7,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  0,  6,  7,  4,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  8,  6,  7,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  2,  8,  5, 11,  2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  0,  4,  5, 11,  2,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  1,  0,  8,  5, 11,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  1,  4,  5, 11,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  4,  3, 11,  2,  8,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  0,  3, 11,  2,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  4,  3, 11,  1,  0,  8,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  1,  3, 11,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  3,  1,  2,  8,  5,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  3,  1,  2,  0,  4,  5,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  3,  0,  8,  5,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  3,  4,  5,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  2,  8,  4,  1,  2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  0,  1,  2,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        {  0,  8,  4,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }
    };

    /** Modifies an octree path by walking in one face direction. */
    bool
    modify_path (CubeFace const& dir, uint8_t level, uint64_t* path)
    {
        bool const subtract = static_cast<int>(dir) % 2;
        int const bit_offset = static_cast<int>(dir) / 2;
        bool overflow = true;
        for (int i = 0; overflow && i < level; ++i)
        {
            uint64_t const mask = uint64_t(1) << (i * 3 + bit_offset);
            if (*path & mask)
            {
                *path ^= mask;
                overflow = !subtract;
            }
            else
            {
                *path |= mask;
                overflow = subtract;
            }
        }

        return !overflow;
    }

    Octree::Iterator
    modify_iterator (CubeFace const& dir, Octree::Iterator const& iter)
    {
        uint64_t path = iter.path;
        if (!modify_path(dir, iter.level, &path))
            return Octree::Iterator();
        return iter.descend(iter.level, path);
    }

    Octree::Iterator
    modify_iterator (CubeFace const& dir1, CubeFace const& dir2,
        Octree::Iterator const& iter)
    {
        uint64_t path = iter.path;
        if (!modify_path(dir1, iter.level, &path))
            return Octree::Iterator();
        if (!modify_path(dir2, iter.level, &path))
            return Octree::Iterator();
        return iter.descend(iter.level, path);
    }
}

mve::TriangleMesh::Ptr
IsoSurface::extract_mesh (void)
{
    std::cout << "  Sanity-checking input data..." << std::flush;
    util::WallTimer timer;
    this->sanity_checks();
    std::cout << " took " << timer.get_elapsed() << " ms." << std::endl;

    /*
     * Assign MC index to every octree node. This can be done in two ways:
     * (1) Iterate all nodes, query corner values, and determine MC index.
     * (2) Iterate leafs only, query corner values, propagate to parents.
     * Strategy (1) is implemented, it is simpler but slightly more expensive.
     */
    std::cout << "  Computing Marching Cubes indices..." << std::flush;
    Octree::Iterator iter = this->octree->get_iterator_for_root();
    for (iter.first_node(); iter.current != nullptr; iter.next_node())
        this->compute_mc_index(iter);
    std::cout << " took " << timer.get_elapsed() << " ms." << std::endl;

    /*
     * Compute isovertices on the octree edges for every leaf node.
     * This locates for every leaf edge the finest unique edge which
     * contains an isovertex. The vertex is stored in the vertex vector,
     * while the edge is stored in the map, mapping edge to vertex ID.
     */
    std::cout << "  Computing isovertices..." << std::flush;
    timer.reset();
    EdgeVertexMap edgemap;
    IsoVertexVector isovertices;
    for (iter.first_leaf(); iter.current != nullptr; iter.next_leaf())
        this->compute_isovertices(iter, &edgemap, &isovertices);
    std::cout << " took " << timer.get_elapsed() << " ms." << std::endl;

    /*
     * Compute polygons for every leaf node. For every leaf face the
     * list of isoedges is retrieved. The isoedges are linked to form
     * one or more closed polygons per node. In some cases open polygons
     * are created, which need to be linked by retrieving twin vertices.
     */
    std::cout << "  Computing isopolygons..." << std::flush;
    timer.reset();
    PolygonList polygons;
    for (iter.first_leaf(); iter.current != nullptr; iter.next_leaf())
        this->compute_isopolygons(iter, edgemap, &polygons);
    std::cout << " took " << timer.get_elapsed() << " ms." << std::endl;

    /*
     * The vertices are transferred to a mesh and the polygons are
     * triangulated using the minimum area triangulation.
     */
    std::cout << "  Computing triangulation..." << std::flush;
    timer.reset();
    mve::TriangleMesh::Ptr mesh = mve::TriangleMesh::create();
    this->compute_triangulation(isovertices, polygons, mesh);
    std::cout << " took " << timer.get_elapsed() << " ms." << std::endl;

    return mesh;
}

void
IsoSurface::sanity_checks (void)
{
    if (this->voxels == nullptr || this->octree == nullptr)
        throw std::runtime_error("sanity_checks(): Null octree/voxels");
    for (std::size_t i = 1; i < this->voxels->size(); ++i)
        if (this->voxels->at(i).first < this->voxels->at(i - 1).first)
            throw std::runtime_error("sanity_checks(): Voxels unsorted");

    Octree::Iterator iter = this->octree->get_iterator_for_root();
    for (iter.first_node(); iter.current != nullptr; iter.next_node())
    {
        /* Check if parent pointer of children is correct. */
        if (iter.current->children == nullptr)
            continue;
        for (int i = 0; i < CUBE_CORNERS; ++i)
        {
            if (iter.current->children[i].parent != iter.current)
                throw std::runtime_error("sanity_checks(): Wrong parent");
        }
    }
}

void
IsoSurface::compute_mc_index (Octree::Iterator const& iter)
{
    iter.current->mc_index = 0;
    for (int i = 0; i < CUBE_CORNERS; ++i)
    {
        VoxelIndex vi;
        vi.from_path_and_corner(iter.level, iter.path, i);
        VoxelData const* vd = this->get_voxel_data(vi);
        if (vd->value < ISO_VALUE)
            iter.current->mc_index |= (1 << i);
    }
}

void
IsoSurface::compute_isovertices (Octree::Iterator const& iter,
    EdgeVertexMap* edgemap, IsoVertexVector* isovertices)
{
    /* This should always be a leaf node. */
    if (iter.current == nullptr || iter.current->children != nullptr)
        throw std::runtime_error("compute_isovertices(): Invalid node");

    /* Check if cube contains an isosurface. */
    if (iter.current->mc_index == 0x00 || iter.current->mc_index == 0xff)
        return;

    for (int i = 0; i < CUBE_EDGES; ++i)
    {
        /* Check if edge contains an isovertex. */
        if (!this->is_isovertex_on_edge(iter.current->mc_index, i))
            continue;

        /* Get the finest edge that contains an isovertex. */
        EdgeIndex edge_index;
        this->get_finest_cube_edge(iter, i, &edge_index, nullptr);
        if (edgemap->find(edge_index) != edgemap->end())
            continue;

        /* Interpolate isovertex and add index to map. */
        IsoVertex isovertex;
        this->get_isovertex(edge_index, i, &isovertex);
        edgemap->insert(std::make_pair(edge_index, isovertices->size()));
        isovertices->push_back(isovertex);
    }
}

void
IsoSurface::get_finest_cube_edge (Octree::Iterator const& iter,
    int edge_id, EdgeIndex* edge_index, EdgeInfo* edge_info)
{
    if (edge_id < 0 || edge_id > 11)
        throw std::runtime_error("get_finest_cube_edge(): Invalid edge ID");

    if (!this->is_isovertex_on_edge(iter.current->mc_index, edge_id))
        throw std::runtime_error("get_finest_cube_edge(): Invalid isoedge");

    /* Remember the finest node found so far. */
    Octree::Iterator finest_iter = iter;
    int finest_edge_id = edge_id;
    bool found_node = false;

    /* Check if the current node has children. */
    if (iter.current->children != nullptr)
        found_node = true;

    /* Check if the two face-adjacent nodes have children. */
    for (int i = 0; !found_node && i < 2; ++i)
    {
        Octree::Iterator temp_iter;
        temp_iter = modify_iterator(edge_neighbors[edge_id][i], iter);
        if (temp_iter.current != nullptr && temp_iter.current->children != nullptr)
        {
            found_node = true;
            finest_iter = temp_iter;
            finest_edge_id = edge_reflections[edge_id][i];
        }
    }

    /* Check if edge-adjacent node has children. */
    if (found_node == false)
    {
        Octree::Iterator temp_iter = modify_iterator(
            edge_neighbors[edge_id][0], edge_neighbors[edge_id][1], iter);
        if (temp_iter.current != nullptr && temp_iter.current->children != nullptr)
        {
            found_node = true;
            finest_iter = temp_iter;
            finest_edge_id = edge_reflections[edge_id][2];
        }
    }

    if (finest_iter.current == nullptr)
        throw std::runtime_error("get_finest_cube_edge(): Error finding edge");

    /* If the node has no children, we found the finest node. */
    if (finest_iter.current->children == nullptr)
    {
        if (edge_index != nullptr)
        {
            VoxelIndex vi1, vi2;
            vi1.from_path_and_corner(finest_iter.level, finest_iter.path,
                edge_corners[finest_edge_id][0]);
            vi2.from_path_and_corner(finest_iter.level, finest_iter.path,
                edge_corners[finest_edge_id][1]);
            edge_index->first = std::min(vi1.index, vi2.index);
            edge_index->second = std::max(vi1.index, vi2.index);
        }
        if (edge_info != nullptr)
        {
            edge_info->iter = finest_iter;
            edge_info->edge_id = finest_edge_id;
        }

        return;
    }

    /* Find unique child with isoedge and recurse. */
    int const child_1_id = edge_corners[finest_edge_id][0];
    int const child_2_id = edge_corners[finest_edge_id][1];
    Octree::Node const& child1 = finest_iter.current->children[child_1_id];
    Octree::Node const& child2 = finest_iter.current->children[child_2_id];
    bool c1_iso = this->is_isovertex_on_edge(child1.mc_index, finest_edge_id);
    bool c2_iso = this->is_isovertex_on_edge(child2.mc_index, finest_edge_id);
    if (c1_iso && !c2_iso)
        this->get_finest_cube_edge(finest_iter.descend(child_1_id),
            finest_edge_id, edge_index, edge_info);
    else if (c2_iso && !c1_iso)
        this->get_finest_cube_edge(finest_iter.descend(child_2_id),
            finest_edge_id, edge_index, edge_info);
    else
        throw std::runtime_error("get_finest_cube_edge(): Invalid parent edge");
}

void
IsoSurface::get_isovertex (EdgeIndex const& edge_index,
    int edge_id, IsoVertex* iso_vertex)
{
    /* Get voxel data. */
    VoxelIndex vi1, vi2;
    vi1.index = edge_index.first;
    vi2.index = edge_index.second;

#if FSSR_USE_DERIVATIVES

    int const edge_axis = edge_id % 3;
    if ((edge_axis == 0 && vi1.get_offset_x() > vi2.get_offset_x())
        || (edge_axis == 1 && vi1.get_offset_y() > vi2.get_offset_y())
        || (edge_axis == 2 && vi1.get_offset_z() > vi2.get_offset_z()))
        std::swap(vi1, vi2);

#endif // FSSR_USE_DERIVATIVES

    VoxelData const* vd1 = this->get_voxel_data(vi1);
    VoxelData const* vd2 = this->get_voxel_data(vi2);
    /* Get voxel positions. */
    math::Vec3d pos1 = vi1.compute_position(
        this->octree->get_root_node_center(),
        this->octree->get_root_node_size());
    math::Vec3d pos2 = vi2.compute_position(
        this->octree->get_root_node_center(),
        this->octree->get_root_node_size());

#if FSSR_USE_DERIVATIVES

    /* Interpolate voxel data and position. */
    double const norm = pos2[edge_axis] - pos1[edge_axis];
    double const weight = interpolate_root(
        vd1->value - ISO_VALUE, vd2->value - ISO_VALUE,
        vd1->deriv[edge_axis] * norm, vd2->deriv[edge_axis] * norm,
        this->interpolation_type);

#else // FSSR_USE_DERIVATIVES

    /* Interpolate voxel data and position. */
    double const weight = (vd1->value - ISO_VALUE) / (vd1->value - vd2->value);

#endif // FSSR_USE_DERIVATIVES

    iso_vertex->data = interpolate_voxel(*vd1, (1.0 - weight), *vd2,  weight);
    iso_vertex->pos = pos1 * (1.0 - weight) + pos2 * weight;
}

bool
IsoSurface::is_isovertex_on_edge (int mc_index, int edge_id)
{
    int const* bit = edge_corners[edge_id];
    return ((mc_index >> bit[0]) & 1) ^ ((mc_index >> bit[1]) & 1);
}

void
IsoSurface::compute_isopolygons (Octree::Iterator const& iter,
    EdgeVertexMap const& edgemap, PolygonList* polygons)
{
    /*
     * Step 1: Collect iso edges for all faces of this node.
     */
    IsoEdgeList isoedges;
    for (int i = 0; i < CUBE_FACES; ++i)
        this->get_finest_isoedges(iter, i, &isoedges, false);

    /* Even cubes with MC index 0x0 or 0xff can have isoedges on the faces. */
    if (isoedges.empty())
        return;

    /*
     * Step 2: Find open vertices by computing vertex valences.
     */
    std::map<EdgeIndex, int> vertex_valence;
    for (std::size_t i = 0; i < isoedges.size(); ++i)
    {
        std::map<EdgeIndex, int>::iterator valence_iter;
        valence_iter = vertex_valence.find(isoedges[i].first);
        if (valence_iter == vertex_valence.end())
            vertex_valence[isoedges[i].first] = 1;
        else
            valence_iter->second += 1;
        valence_iter = vertex_valence.find(isoedges[i].second);
        if (valence_iter == vertex_valence.end())
            vertex_valence[isoedges[i].second] = -1;
        else
            valence_iter->second -= 1;
    }

    /*
     * Step 3: Close open polygons by connecting open twin vertices.
     */
    for (std::size_t i = 0; i < isoedges.size(); ++i)
    {
        IsoEdge const& isoedge = isoedges[i];
        if (vertex_valence[isoedge.first] != 0)
        {
            EdgeIndex twin;
            EdgeInfo twin_info;
            this->find_twin_vertex(isoedge.first_info, &twin, &twin_info);

            IsoEdge new_edge;
            new_edge.first = twin;
            new_edge.first_info = twin_info;
            new_edge.second = isoedge.first;
            new_edge.second_info = isoedge.first_info;
            isoedges.push_back(new_edge);

            vertex_valence[new_edge.first] += 1;
            vertex_valence[new_edge.second] -= 1;
        }

        if (vertex_valence[isoedge.second] != 0)
        {
            EdgeIndex twin;
            EdgeInfo twin_info;
            this->find_twin_vertex(isoedge.second_info, &twin, &twin_info);

            IsoEdge new_edge;
            new_edge.first = isoedges[i].second;
            new_edge.first_info = isoedges[i].second_info;
            new_edge.second = twin;
            new_edge.second_info = twin_info;
            isoedges.push_back(new_edge);

            vertex_valence[new_edge.first] += 1;
            vertex_valence[new_edge.second] -= 1;
        }
    }

    /*
     * Step 4: Join edges to form closed polygons.
     */
    std::size_t poly_start = 0;
    for (std::size_t i = 0; i < isoedges.size(); ++i)
    {
        /* Once joined edges close, issue a new polygon. */
        if (isoedges[i].second == isoedges[poly_start].first)
        {
            polygons->push_back(std::vector<std::size_t>());
            std::vector<std::size_t>& poly = polygons->back();
            for (std::size_t j = poly_start; j <= i; ++j)
            {
                poly.push_back(this->lookup_edge_vertex(edgemap,
                    isoedges[j].first));
            }
            poly_start = i + 1;
            continue;
        }

        /* Fix successive edge. */
        bool found_edge = false;
        for (std::size_t j = i + 1; !found_edge && j < isoedges.size(); ++j)
        {
            if (isoedges[i].second == isoedges[j].first)
            {
                std::swap(isoedges[i + 1], isoedges[j]);
                found_edge = true;
            }
        }

        if (!found_edge)
            throw std::runtime_error("Cannot find next edge");
    }
}

std::size_t
IsoSurface::lookup_edge_vertex (EdgeVertexMap const& edgemap,
    EdgeIndex const& edge)
{
    EdgeVertexMap::const_iterator iter = edgemap.find(edge);
    if (iter == edgemap.end())
        throw std::runtime_error("lookup_edge_vertex(): No such edge vertex");
    return iter->second;
}

void
IsoSurface::find_twin_vertex (EdgeInfo const& edge_info,
    EdgeIndex* twin, EdgeInfo* twin_info)
{
    /*
     * Find the twin isovertex for the given edge. Go upwards in the tree
     * through the parent edges until an edge with no iso crossing is found.
     * Get the twin isovertex on the second child adjacent to the edge.
     */
    Octree::Iterator iter = edge_info.iter;
    int const edge_id = edge_info.edge_id;
    while (iter.current->parent != nullptr)
    {
        int const node_octant
            = static_cast<int>(iter.current - iter.current->parent->children);

        /* The octant of this node in the parent must be on the same edge. */
        int descend_octant;
        if (edge_corners[edge_id][0] == node_octant)
            descend_octant = edge_corners[edge_id][1];
        else if (edge_corners[edge_id][1] == node_octant)
            descend_octant = edge_corners[edge_id][0];
        else
            throw std::runtime_error("find_twin_vertex(): Invalid parent edge");

        /* If the parent edge has no isocrossing, descend to find twin. */
        iter = iter.ascend();
        if (!this->is_isovertex_on_edge(iter.current->mc_index, edge_id))
        {
            iter = iter.descend(descend_octant);
            this->get_finest_cube_edge(iter, edge_id, twin, twin_info);
            return;
        }
    }

    throw std::runtime_error("find_twin_vertex(): Reached octree root");

}

void
IsoSurface::get_finest_isoedges (Octree::Iterator const& iter,
    int face_id, IsoEdgeList* isoedges, bool descend_only)
{
    /* If descend only is set, face-neighboring nodes are not considered. */
    if (descend_only)
    {
        if (iter.current->children != nullptr)
        {
            /* Recursively descend to obtain iso edges for this face. */
            this->get_finest_isoedges(iter.descend(face_corners[face_id][0]),
                face_id, isoedges, true);
            this->get_finest_isoedges(iter.descend(face_corners[face_id][1]),
                face_id, isoedges, true);
            this->get_finest_isoedges(iter.descend(face_corners[face_id][2]),
                face_id, isoedges, true);
            this->get_finest_isoedges(iter.descend(face_corners[face_id][3]),
                face_id, isoedges, true);
        }
        else
        {
            /* Create list of isoedges for this face. */
            int const mc_index = iter.current->mc_index;
            int const* edge_table = mc_polygons[mc_index];

            int first_id = -1;
            for (int i = 0; edge_table[i] != -1; ++i)
            {
                if (first_id == -1)
                {
                    first_id = edge_table[i];
                    continue;
                }
                if (edge_table[i] == first_id)
                    first_id = -1;

                if (!is_edge_on_face[edge_table[i - 1]][face_id]
                    || !is_edge_on_face[edge_table[i]][face_id])
                    continue;

                IsoEdge isoedge;
                this->get_finest_cube_edge(iter, edge_table[i - 1],
                    &isoedge.first, &isoedge.first_info);
                this->get_finest_cube_edge(iter, edge_table[i],
                    &isoedge.second, &isoedge.second_info);
                isoedges->push_back(isoedge);
            }
        }

        return;
    }

    /* Check if face-neighboring node has finer subdivision. */
    Octree::Iterator niter = modify_iterator((CubeFace)face_id, iter);
    if (niter.current != nullptr && niter.current->children != nullptr)
    {
        /* Face-neighboring node has finer subdivision. */
        CubeFace const opposite_face_id = face_opposite[face_id];
        std::size_t const last_isoedge_index = isoedges->size();
        this->get_finest_isoedges(niter, opposite_face_id, isoedges, true);

        /* Flip orientation for face-neighboring iso-edges. */
        for (std::size_t i = last_isoedge_index; i < isoedges->size(); ++i)
        {
            IsoEdge& isoedge = isoedges->at(i);
            std::swap(isoedge.first, isoedge.second);
            std::swap(isoedge.first_info, isoedge.second_info);
        }
    }
    else
    {
        /* Find the isoedges for this node face. */
        this->get_finest_isoedges(iter, face_id, isoedges, true);
    }
}

void
IsoSurface::compute_triangulation(IsoVertexVector const& isovertices,
    PolygonList const& polygons, mve::TriangleMesh::Ptr mesh)
{
    /* Populate per-vertex attributes. */
    mve::TriangleMesh::VertexList& verts = mesh->get_vertices();
    mve::TriangleMesh::ColorList& colors = mesh->get_vertex_colors();
    mve::TriangleMesh::ValueList& values = mesh->get_vertex_values();
    mve::TriangleMesh::ConfidenceList& cfs = mesh->get_vertex_confidences();
    verts.reserve(isovertices.size());
    colors.reserve(isovertices.size());
    values.reserve(isovertices.size());
    cfs.reserve(isovertices.size());

    for (std::size_t i = 0; i < isovertices.size(); ++i)
    {
        IsoVertex const& vertex = isovertices[i];
        verts.push_back(vertex.pos);
        colors.push_back(math::Vec4f(vertex.data.color, 1.0f));
        values.push_back(vertex.data.scale);
        cfs.push_back(vertex.data.conf);
    }

    /* Triangulate isopolygons. */
    mve::TriangleMesh::FaceList& triangles = mesh->get_faces();
    fssr::MinAreaTriangulation tri;
    for (std::size_t i = 0; i < polygons.size(); i++)
    {
        std::vector<math::Vector<float, 3> > loop;
        loop.resize(polygons[i].size());
        for (std::size_t j = 0; j < polygons[i].size(); ++j)
            loop[j] = verts[polygons[i][j]];
        std::vector<unsigned int> result;
        tri.triangulate(loop, &result);
        for (std::size_t j = 0; j < result.size(); ++j)
            triangles.push_back(polygons[i][result[j]]);
    }
}

FSSR_NAMESPACE_END
