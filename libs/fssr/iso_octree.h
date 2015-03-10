/*
 * This file is part of the Floating Scale Surface Reconstruction software.
 * Written by Simon Fuhrmann.
 */

#ifndef FSSR_ISO_OCTREE_HEADER
#define FSSR_ISO_OCTREE_HEADER

#include <vector>

#include "fssr/defines.h"
#include "fssr/voxel.h"
#include "fssr/octree.h"

FSSR_NAMESPACE_BEGIN

/**
 * This class computes the implicit function by querying function values
 * at the octree primal vertices of the leaf nodes, called voxels.
 */
class IsoOctree : public Octree
{
public:
    typedef std::vector<std::pair<VoxelIndex, VoxelData> > VoxelVector;

public:
    IsoOctree (void);

    /** Resets the octree to its initial state. */
    void clear (void);

    /** Clears the voxel data, keeps samples and hierarchy. */
    void clear_voxel_data (void);

    /** Evaluate the implicit function for all voxels on all leaf nodes. */
    void compute_voxels (void);

    /** Returns the map of computed voxels. */
    VoxelVector const& get_voxels (void) const;

private:
    void compute_all_voxels (void);
    VoxelData sample_ifn (math::Vec3d const& voxel_pos);
    void print_progress (std::size_t voxels_done, std::size_t voxels_total);

private:
    VoxelVector voxels;
};

FSSR_NAMESPACE_END

/* ------------------------- Implementation ---------------------------- */

FSSR_NAMESPACE_BEGIN

inline
IsoOctree::IsoOctree (void)
{
}

inline void
IsoOctree::clear (void)
{
    this->clear_voxel_data();
    this->Octree::clear();
}

inline void
IsoOctree::clear_voxel_data (void)
{
    this->voxels.clear();
}

inline IsoOctree::VoxelVector const&
IsoOctree::get_voxels (void) const
{
    return this->voxels;
}

FSSR_NAMESPACE_END

#endif /* FSSR_ISO_OCTREE_HEADER */
