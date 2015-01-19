/*
 * This file is part of the Floating Scale Surface Reconstruction software.
 * Written by Simon Fuhrmann.
 */

#ifndef FSSR_VOXEL_HEADER
#define FSSR_VOXEL_HEADER

#include <stdint.h>  // TODO: Use <cstdint> once C++11 is standard.

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
    void from_path_and_corner (uint8_t level, uint64_t path, int corner);
    math::Vec3d compute_position (math::Vec3d const& center, double size);
    uint32_t get_offset_x (void) const;
    uint32_t get_offset_y (void) const;
    uint32_t get_offset_z (void) const;
    bool operator< (VoxelIndex const& other) const;

public:
    uint64_t index;
};

/* --------------------------------------------------------------------- */

/**
 * Stores per voxel data. The is the actual SDF/implicit function value,
 * a confidence value and the cumulative color.
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
};

/* --------------------------------------------------------------------- */

/**
 * Interpolates between two VoxelData objects for Marching Cubes.
 * The interpolation linearly interpolates value and color, and uses
 * the minimum of the confidence.
 */
VoxelData
interpolate (VoxelData const& d1, float w1, VoxelData const& d2, float w2);

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
