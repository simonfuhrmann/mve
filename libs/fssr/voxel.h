/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef FSSR_VOXEL_HEADER
#define FSSR_VOXEL_HEADER

#include <cstdint>

#include "fssr/defines.h"
#include "fssr/octree.h"

FSSR_NAMESPACE_BEGIN

/**
 * The voxel index is a unique 64 bit ID for each voxel in the octree.
 * The index is designed in such a way that it is independent of the
 * level, i.e., neighboring nodes on different levels share voxels with
 * the same ID. The bits are assigned as follows:
 *
 *      0      000...000  000...000  000...000
 *   --------  ---------  ---------  ---------
 *   1 unused   21 bits    21 bits    21 bits
 *      bit     z-coord    y-coord    x-coord
 *
 * Since the maximum voxel index for level L is 2^L, this limits
 * the maximum level to 20 with 21 bits, i.e. 2^20-1 < 2^20 < 2^21-1.
 * Voxels at a lower level are shifted to the hightest bit to obtain the
 * same index over different levels. For example, index I on level L is
 * shifted I << (20 - L).
 */
struct VoxelIndex
{
public:
    /** Computes the octree corner index given octree node path and level. */
    void from_path_and_corner (uint8_t level, uint64_t path, int corner);
    /** Computes the position of a voxel given octree root size and center. */
    math::Vec3d compute_position (math::Vec3d const& center, double size) const;
    int32_t get_offset_x (void) const;
    int32_t get_offset_y (void) const;
    int32_t get_offset_z (void) const;
    bool operator< (VoxelIndex const& other) const;

public:
    uint64_t index;
};

/* --------------------------------------------------------------------- */

/**
 * Stores per voxel data. The is the actual SDF/implicit function value,
 * a confidence value and the cumulative color. The scale value is mainly
 * interesting to store scale information in the output mesh.
 */
struct VoxelData
{
public:
    VoxelData (void);

public:
    float value;
    float conf;
    float scale;
    math::Vec3f color;
#if FSSR_USE_DERIVATIVES
    math::Vec3f deriv;
#endif // FSSR_USE_DERIVATIVES
};

/* --------------------------------------------------------------------- */

/**
 * Interpolates between two VoxelData objects for Marching Cubes.
 * The specified weights 'w1' and 'w2' are used for interpolation of value,
 * scale and color. For the confidence, however, the minimum value is used.
 */
VoxelData
interpolate_voxel (VoxelData const& d1, float w1,
    VoxelData const& d2, float w2);

FSSR_NAMESPACE_END

/* ------------------------- Implementation ---------------------------- */

FSSR_NAMESPACE_BEGIN

inline bool
VoxelIndex::operator< (VoxelIndex const& other) const
{
    return this->index < other.index;
}

inline
VoxelData::VoxelData (void)
    : value(0.0f)
    , conf(0.0f)
    , scale(0.0f)
    , color(0.0f)
{
}

FSSR_NAMESPACE_END

#endif /* FSSR_VOXEL_HEADER */
