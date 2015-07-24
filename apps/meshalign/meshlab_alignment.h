/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef MESHLAB_DATASET_HEADER
#define MESHLAB_DATASET_HEADER

#include <vector>

#include "mve/mesh.h"
#include "range_image.h"

/**
 * Reads a Meshlab alignment file and performs the transformation.
 * Meshlab alignment file format:
 *
 * NUM_MESHES
 *
 * MESH_FILE_NAME
 * R1 R2 R3 T1
 * R4 R5 R6 T2
 * R7 R8 R9 T3
 * 0  0  0  1
 *
 * MESH_FILE_NAME
 * ...
 */
class MeshlabAlignment
{
public:
    typedef std::vector<RangeImage> RangeImages;

public:
    MeshlabAlignment (void);

    void read_alignment (std::string const& filename);
    RangeImages const& get_range_images (void) const;
    mve::TriangleMesh::Ptr get_mesh (RangeImage const& ri) const;

private:
    RangeImages images;
};

/* ---------------------------------------------------------------- */

inline
MeshlabAlignment::MeshlabAlignment (void)
{
}

inline MeshlabAlignment::RangeImages const&
MeshlabAlignment::get_range_images (void) const
{
    return this->images;
}

#endif // MESHLAB_DATASET_H
