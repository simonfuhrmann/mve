/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <stdexcept>

#include "math/geometry.h"
#include "fssr/defines.h"
#include "fssr/triangulation.h"

FSSR_NAMESPACE_BEGIN

void
MinAreaTriangulation::triangulate (std::vector<math::Vec3f> const& verts,
        std::vector<unsigned int>* indices)
{
    if (verts.size() < 3)
        throw std::invalid_argument("Invalid polygon with <3 vertices");

    if (verts.size() == 3)
    {
        indices->push_back(0);
        indices->push_back(1);
        indices->push_back(2);
        return;
    }

    this->min_area_table.clear();
    this->mid_point_table.clear();
    this->min_area_table.resize(verts.size() * verts.size(), -1.0f);
    this->mid_point_table.resize(verts.size() * verts.size(), -1);
    this->compute_table(verts, 0, 1);
    indices->clear();
    this->compute_triangulation(indices, 0, 1, verts.size());
}

float
MinAreaTriangulation::compute_table (std::vector<math::Vec3f> const& verts,
    int start_id, int end_id)
{
    if (start_id == end_id)
        return 0.0f;
    if (start_id == (end_id + 1) % (int)verts.size())
        return 0.0f;

    int const index = start_id * verts.size() + end_id;
    float& min_area = this->min_area_table[index];
    if (min_area < 0.0f)
    {
        for (int mid_point = (end_id + 1) % (int)verts.size();
            mid_point != start_id;
            mid_point = (mid_point + 1) % (int)verts.size())
        {
            float temp = this->compute_table(verts, start_id, mid_point)
                + this->compute_table(verts, mid_point, end_id)
                + math::geom::triangle_area(verts[start_id],
                verts[end_id], verts[mid_point]);

            if (temp < min_area || min_area < 0.0f)
            {
                min_area = temp;
                this->mid_point_table[index] = mid_point;
            }
       }
   }

   return min_area;
}

void
MinAreaTriangulation::compute_triangulation (std::vector<unsigned int>* indices,
    int start_id, int end_id, int num_verts)
{
    int const index = start_id * num_verts + end_id;
    int mid_point = this->mid_point_table[index];
    if (mid_point < 0)
        return;
    indices->push_back(start_id);
    indices->push_back(end_id);
    indices->push_back(mid_point);
    this->compute_triangulation(indices, start_id, mid_point, num_verts);
    this->compute_triangulation(indices, mid_point, end_id, num_verts);
}

FSSR_NAMESPACE_END
