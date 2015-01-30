/*
 * This file is part of the Floating Scale Surface Reconstruction software.
 * Written by Simon Fuhrmann.
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
    enum FaceDirection
    {
        POSITIVE_X = 0,
        NEGATIVE_X = 1,
        POSITIVE_Y = 2,
        NEGATIVE_Y = 3,
        POSITIVE_Z = 4,
        NEGATIVE_Z = 5
    };

    FaceDirection const face_opposite[6] =
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

    /** The directions of neighboring nodes around an edge. */
    FaceDirection const edge_neighbors[12][2] =
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
    modify_path (FaceDirection const& dir, uint8_t level, uint64_t* path)
    {
        bool const subtract = static_cast<int>(dir) % 2;
        int const bit_offset = static_cast<int>(dir) / 2;
        bool overflow = true;
        for (int i = 0; overflow && i < level; ++i)
        {
            int const mask = 1 << (i * 3 + bit_offset);
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
}

mve::TriangleMesh::Ptr
IsoSurface::extract_mesh (void)
{
    std::cout << "  Sanity-checking input data..." << std::endl;
    this->sanity_checks();
    util::WallTimer timer;

    /*
     * Assign MC index to every octree node. This can be done in two ways:
     * (1) Iterate all nodes, query corner values, and determine MC index.
     * (2) Iterate leafs only, query corner values, propagate to parents.
     * Strategy (1) is implemented, it is simpler but slightly more expensive.
     */
    std::cout << "  Computing Marching Cubes indices..." << std::flush;
    Octree::Iterator iter = this->octree->get_iterator_for_root();
    for (iter.first_node(); iter.current != NULL; iter.next_node())
        this->compute_mc_index(iter);
    std::cout << " took " << timer.get_elapsed() << " ms." << std::endl;

    /*
     * Compute isovertices on the octree edges for every leaf node.
     */
    std::cout << "  Computing isovertices..." << std::flush;
    timer.reset();
    EdgeVertexMap edgemap;
    IsoVertexVector isovertices;
    for (iter.first_leaf(); iter.current != NULL; iter.next_leaf())
        this->compute_isovertices(iter, &edgemap, &isovertices);
    std::cout << " took " << timer.get_elapsed() << " ms." << std::endl;

    /*
     * Compute polygons for every leaf node.
     */
    std::cout << "  Computing isopolygons..." << std::flush;
    timer.reset();
    PolygonList polygons;
    for (iter.first_leaf(); iter.current != NULL; iter.next_leaf())
        this->compute_isopolygons(iter, edgemap, &polygons);
    std::cout << " took " << timer.get_elapsed() << " ms." << std::endl;

    /*
     * Init the mesh with positions, colors, confidences, and scale values.
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
    if (this->voxels == NULL || this->octree == NULL)
        throw std::runtime_error("extract_isosurface(): NULL octree/voxels");
    for (std::size_t i = 1; i < this->voxels->size(); ++i)
        if (this->voxels->at(i).first < this->voxels->at(i - 1).first)
            throw std::runtime_error("extract_isosurface(): Voxels unsorted");
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
    if (iter.current == NULL || iter.current->children != NULL)
        throw std::runtime_error("compute_isovertices(): Invalid node");

    /* Check if cube contains an isosurface. */
    if (iter.current->mc_index == 0x00 || iter.current->mc_index == 0xff)
        return;

#if 0 // Hack: Create polygons within each cube
    for (int i = 0; i < CUBE_EDGES; ++i)
    {
        /* Check if edge contains an isovertex. */
        if (!this->is_isovertex_on_edge(iter.current->mc_index, i))
            continue;

        VoxelIndex vi1, vi2;
        vi1.from_path_and_corner(iter.level, iter.path, edge_corners[i][0]);
        vi2.from_path_and_corner(iter.level, iter.path, edge_corners[i][1]);
        EdgeIndex edge_index;
        edge_index.first = std::min(vi1.index, vi2.index);
        edge_index.second = std::max(vi1.index, vi2.index);

        if (edgemap->find(edge_index) == edgemap->end())
        {
            /* Interpolate isovertex and add index to map. */
            IsoVertex isovertex;
            this->get_isovertex(edge_index, &isovertex);
            edgemap->insert(std::make_pair(edge_index, isovertices->size()));
            isovertices->push_back(isovertex);
        }
    }

    return;
#endif

    for (int i = 0; i < CUBE_EDGES; ++i)
    {
        /* Check if edge contains an isovertex. */
        if (!this->is_isovertex_on_edge(iter.current->mc_index, i))
            continue;

        /* Get the finest edge that contains an isovertex. */
        EdgeIndex edge_index;
        this->get_finest_cube_edge(iter, i, &edge_index);
        if (edgemap->find(edge_index) != edgemap->end())
            continue;

        /* Interpolate isovertex and add index to map. */
        IsoVertex isovertex;
        this->get_isovertex(edge_index, &isovertex);
        edgemap->insert(std::make_pair(edge_index, isovertices->size()));
        isovertices->push_back(isovertex);
    }
}

void
IsoSurface::get_finest_cube_edge (Octree::Iterator const& iter,
    int edge_id, EdgeIndex* edge_index)
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
    if (iter.current->children != NULL)
        found_node = true;

    /* Check if first face-adjacent node has children. */
    uint64_t temp_path = iter.path;
    if (found_node == false
        && modify_path(edge_neighbors[edge_id][0], iter.level, &temp_path))
    {
        Octree::Iterator temp_iter = iter.descend(iter.level, temp_path);
        if (temp_iter.current != NULL && temp_iter.current->children != NULL)
        {
            found_node = true;
            finest_iter = temp_iter;
            finest_edge_id = edge_reflections[edge_id][0];
        }
    }

    /* Check if second face-adjacent node has children. */
    temp_path = iter.path;
    if (found_node == false
        && modify_path(edge_neighbors[edge_id][1], iter.level, &temp_path))
    {
        Octree::Iterator temp_iter = iter.descend(iter.level, temp_path);
        if (temp_iter.current != NULL && temp_iter.current->children != NULL)
        {
            found_node = true;
            finest_iter = temp_iter;
            finest_edge_id = edge_reflections[edge_id][1];
        }
    }

    /* Check if edge-adjacent node has children. */
    temp_path = iter.path;
    if (found_node == false
        && modify_path(edge_neighbors[edge_id][0], iter.level, &temp_path)
        && modify_path(edge_neighbors[edge_id][1], iter.level, &temp_path))
    {
        Octree::Iterator temp_iter = iter.descend(iter.level, temp_path);
        if (temp_iter.current != NULL && temp_iter.current->children != NULL)
        {
            found_node = true;
            finest_iter = temp_iter;
            finest_edge_id = edge_reflections[edge_id][2];
        }
    }

    if (finest_iter.current == NULL)
        throw std::runtime_error("get_finest_cube_edge(): Error finding edge");

    /* If the finest node has no children, it is the finest node. */
    if (finest_iter.current->children == NULL)
    {
        VoxelIndex vi1, vi2;
        vi1.from_path_and_corner(finest_iter.level, finest_iter.path,
            edge_corners[finest_edge_id][0]);
        vi2.from_path_and_corner(finest_iter.level, finest_iter.path,
            edge_corners[finest_edge_id][1]);
        edge_index->first = std::min(vi1.index, vi2.index);
        edge_index->second = std::max(vi1.index, vi2.index);
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
            finest_edge_id, edge_index);
    else if (c2_iso && !c1_iso)
        this->get_finest_cube_edge(finest_iter.descend(child_2_id),
            finest_edge_id, edge_index);
    else
        throw std::runtime_error("get_finest_cube_edge(): Invalid parent edge");
}

void
IsoSurface::get_isovertex (EdgeIndex const& edge_index, IsoVertex* iso_vertex)
{
    /* Get voxel data. */
    VoxelIndex vi1, vi2;
    vi1.index = edge_index.first;
    vi2.index = edge_index.second;
    VoxelData const* vd1 = this->get_voxel_data(vi1);
    VoxelData const* vd2 = this->get_voxel_data(vi2);
    /* Get voxel positions. */
    math::Vec3d pos1 = vi1.compute_position(
        this->octree->get_root_node_center(),
        this->octree->get_root_node_size());
    math::Vec3d pos2 = vi2.compute_position(
        this->octree->get_root_node_center(),
        this->octree->get_root_node_size());
    /* Interpolate voxel data and position. */
    double const w = (vd1->value - ISO_VALUE) / (vd1->value - vd2->value);
    iso_vertex->data = fssr::interpolate(*vd1, (1.0 - w), *vd2,  w);
    iso_vertex->pos = pos1 * (1.0 - w) + pos2 * w;
    iso_vertex->orientation = (vd1->value < ISO_VALUE);
}

bool
IsoSurface::is_isovertex_on_edge (int mc_index, int edge_id)
{
    int const* bit = edge_corners[edge_id];
    return ((mc_index >> bit[0]) & 1) ^ ((mc_index >> bit[1]) & 1);
}

void
IsoSurface::compute_isopolygons(Octree::Iterator const& iter,
    EdgeVertexMap const& edgemap,
    std::vector<std::vector<int> >* polygons)
{
    /*
     *  Step 1: Collect iso edges for this node
     *   Iterate over all 6 faces of the current node
     *   If face neighboring nodes have finer levels
     *   - Use iso edges from finer levels
     *   - Else use iso edges from this node face
     *
     * Step 2: Find open vertices with valence one
     *   Create map for each iso edge endpoint and count incoming - outgoing
     *   Fix vertices with valence one by connecting with twin vertex
     *
     * Step 3: Join edges to form closed polygons
     *   Start with an edge and chain edges while available
     *   Check for remaining edges and start over
     */

    /* Step 1: Collect iso edges. */
    IsoEdgeList isoedges;
    for (int i = 0; i < CUBE_FACES; ++i)
        this->get_finest_isoedges (iter, i, &isoedges);


#if 0 // Hack: Create polygons within each cube
    /* Check if cube contains an isosurface. */
    if (iter.current->mc_index == 0x00 || iter.current->mc_index == 0xff)
        return;

    int const* polygon_spec = mc_polygons[iter.current->mc_index];
    int first_id = -1;
    for (int i = 0; polygon_spec[i] != -1; ++i)
    {
        int const edge_id = polygon_spec[i];

        if (edge_id == first_id)
        {
            first_id = -1;
            continue;
        }

        if (first_id == -1)
        {
            first_id = edge_id;
            polygons->push_back(std::vector<int>());
        }

        VoxelIndex vi1, vi2;
        vi1.from_path_and_corner(iter.level, iter.path, edge_corners[edge_id][0]);
        vi2.from_path_and_corner(iter.level, iter.path, edge_corners[edge_id][1]);

        EdgeIndex edge_index;
        edge_index.first = std::min(vi1.index, vi2.index);
        edge_index.second = std::max(vi1.index, vi2.index);

        EdgeVertexMap::const_iterator iter = edgemap.find(edge_index);
        if (iter == edgemap.end())
            throw std::runtime_error("No such edge");
        //std::size_t const index = edgemap[edge_index];
        std::size_t const index = iter->second;
        polygons->back().push_back(index);
    }
#endif
}

void
IsoSurface::get_finest_isoedges (Octree::Iterator const& iter,
    int face_id, IsoEdgeList* isoedges)
{
    uint64_t new_path = iter.path;
    modify_path((FaceDirection)face_id, iter.level, &new_path);
    Octree::Iterator niter = iter.descend(iter.level, new_path);

    if (niter.current != NULL && niter.current->children != NULL)
    {
        /* Neighboring node has finer subdivision. Use those eges. */
        /* Flip those edges afterwards. */
    }
    else
    {
        /* Use the isoedges from this node face. */
    }
}




void
IsoSurface::compute_triangulation(IsoVertexVector const& isovertices,
    std::vector<std::vector<int> > const& polygons,
    mve::TriangleMesh::Ptr mesh)
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
    for (size_t i = 0; i < polygons.size(); i++)
    {
        std::vector<math::Vector<float, 3> > loop;
        loop.resize(polygons[i].size());
        for (size_t j = 0; j < polygons[i].size(); ++j)
            loop[j] = verts[polygons[i][j]];
        std::vector<unsigned int> result;
        tri.triangulate(loop, &result);
        for (std::size_t j = 0; j < result.size(); ++j)
            triangles.push_back(polygons[i][result[j]]);
    }
}

FSSR_NAMESPACE_END
