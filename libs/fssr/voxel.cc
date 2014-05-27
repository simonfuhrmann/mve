/*
 * This file is part of the Floating Scale Surface Reconstruction software.
 * Written by Simon Fuhrmann.
 */

#include <algorithm>

#include "math/vector.h"
#include "fssr/voxel.h"

FSSR_NAMESPACE_BEGIN

void
VoxelIndex::from_path_and_corner (Octree::NodePath const& path, int corner)
{
    /* Compute node index from the node path. */
    math::Vector<uint64_t, 3> coords(0, 0, 0);
    for (int l = path.level - 1; l >= 0; --l)
    {
        uint64_t index = (path.path >> (3 * l)) & 7;
        for (int i = 0; i < 3; ++i)
        {
            coords[i] = coords[i] << 1;
            coords[i] += (index & (1 << i)) >> i;
        }
    }

    /* Convert node index to voxel index. */
    for (int i = 0; i < 3; ++i)
    {
        coords[i] += (corner >> i) & 1;
        coords[i] = coords[i] << (20 - path.level);
    }

    /* Compute voxel ID. */
    this->index = coords[0] | coords[1] << 21 | coords[2] << 42;
}

math::Vec3d
VoxelIndex::compute_position (math::Vec3d const& center, double size)
{
    double const dim_max = static_cast<double>(1 << 20);
    double const fx = static_cast<double>(this->get_offset_x()) / dim_max;
    double const fy = static_cast<double>(this->get_offset_y()) / dim_max;
    double const fz = static_cast<double>(this->get_offset_z()) / dim_max;
    return center - size / 2.0 + math::Vec3d(fx, fy, fz) * size;
}

uint32_t
VoxelIndex::get_offset_x (void) const
{
    return static_cast<uint32_t>((this->index >> 0) & 0x1fffff);
}

uint32_t
VoxelIndex::get_offset_y (void) const
{
    return static_cast<uint32_t>((this->index >> 21) & 0x1fffff);
}

uint32_t
VoxelIndex::get_offset_z (void) const
{
    return static_cast<uint32_t>((this->index >> 42) & 0x1fffff);
}

/* ---------------------------------------------------------------- */

void
VoxelIndexWithLevel::from_voxel_id_and_level (uint64_t voxel_id, uint8_t level)
{
    /* 5 bits for the level, the rest for the voxel ID. */
    this->index = (static_cast<uint64_t>(level) << 59 | voxel_id);
}

void
VoxelIndexWithLevel::from_path_and_corner (Octree::NodePath const& path,
    int corner)
{
    /* Compute node index from the node path. */
    math::Vector<uint64_t, 3> coords(0, 0, 0);
    for (int l = path.level - 1; l >= 0; --l)
    {
        uint64_t index = (path.path >> (3 * l)) & 7;
        for (int i = 0; i < 3; ++i)
        {
            coords[i] = coords[i] << 1;
            coords[i] += (index & (1 << i)) >> i;
        }
    }

    /* Convert node index to voxel index. */
    for (int i = 0; i < 3; ++i)
        coords[i] += (corner >> i) & 1;

    /* Compute voxel ID. */
    uint64_t voxel_id = coords[0] | coords[1] << 19 | coords[2] << 38;
    this->from_voxel_id_and_level(voxel_id, path.level);
}

uint8_t
VoxelIndexWithLevel::get_level (void) const
{
    return static_cast<uint8_t>(this->index >> 59);
}

uint32_t
VoxelIndexWithLevel::get_offset_x (void) const
{
    return static_cast<uint32_t>((this->index >> 0) & 0x7ffff);
}

uint32_t
VoxelIndexWithLevel::get_offset_y (void) const
{
    return static_cast<uint32_t>((this->index >> 19) & 0x7ffff);
}

uint32_t
VoxelIndexWithLevel::get_offset_z (void) const
{
    return static_cast<uint32_t>((this->index >> 38) & 0x7ffff);
}

/* ---------------------------------------------------------------- */

VoxelData
interpolate (VoxelData const& d1, float w1, VoxelData const& d2, float w2)
{
    VoxelData ret;
    ret.value = w1 * d1.value + w2 * d2.value;
    ret.conf = std::min(d1.conf, d2.conf);
    ret.scale = w1 * d1.scale + w2 * d2.scale;
    ret.color = w1 * d1.color + w2 * d2.color;
    return ret;
}

FSSR_NAMESPACE_END
