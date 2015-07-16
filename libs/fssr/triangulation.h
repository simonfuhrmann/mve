/*
 * This file is part of the Floating Scale Surface Reconstruction software.
 * Written by Simon Fuhrmann.
 */

#ifndef FSSR_TRIANGULATION_HEADER
#define FSSR_TRIANGULATION_HEADER

#include <vector>

#include "math/vector.h"
#include "fssr/defines.h"

FSSR_NAMESPACE_BEGIN

/**
 * Computes the minimum area triangulation of a polygon.
 * The algorithm is described in the paper:
 *
 *     Unconstrained Isosurface Extraction on Arbitrary Octrees
 *     Michael Kazhdan, Allison Klein, Ketan Dalal, Hugues Hoppe
 */
class MinAreaTriangulation
{
public:
    void triangulate (std::vector<math::Vec3f> const& verts,
        std::vector<unsigned int>* indices);

private:
    float compute_table (std::vector<math::Vec3f> const& verts,
        int start_id, int end_id);
    void compute_triangulation (std::vector<unsigned int>* indices,
        int start_id, int end_id, int num_verts);

private:
    std::vector<float> min_area_table;
    std::vector<int> mid_point_table;
};

/*
 * TODO: Triangulation that creates a center vertex.
 */

FSSR_NAMESPACE_END

#endif /* FSSR_TRIANGULATION_HEADER */
