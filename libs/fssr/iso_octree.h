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
 * Given an octree with samples, this class generates the actual implicit
 * function by querying function values at the octree primal vertices of
 * the leaf nodes. The voxels are stored in a map, which is convenient and
 * simplifies the data structure (because neighboring octree nodes share
 * common voxels).
 */
class IsoOctree : public Octree
{
public:
    typedef std::vector<std::pair<VoxelIndex, VoxelData> > VoxelVector;

public:
    IsoOctree (void);

    /** Evaluate the implicit function for all voxels on all leaf nodes. */
    void compute_voxels (void);

    /**
     * Sets the maximum level on which voxels are generated. See voxel.h.
     * The default is 19, deeper levels are not supported by Kazhdans code.
     */
    void set_max_level (int max_level);

    /**
     * Returns the maximum level on which voxels are generated.
     * The root level is 0, children are at level 1, and so on.
     */
    int get_max_level (void) const;

    /** Returns the map of computed voxels. */
    VoxelVector const& get_voxels (void) const;
    /** Cleas the octree and voxels. */
    void clear (void);

    /** Writes the voxel data and octree hierarchy to file. */
    void write_to_file (std::string const& filename) const;
    /** Reads the voxel data and octree hierarchy from file. */
    void read_from_file (std::string const& filename);

private:
    void compute_all_voxels (void);
    VoxelData sample_ifn (math::Vec3d const& voxel_pos);
    void print_progress (std::size_t voxels_done, std::size_t voxels_total);

private:
    int max_level;
    VoxelVector voxels;
    std::size_t num_processed;
};

FSSR_NAMESPACE_END

/* ------------------------- Implementation ---------------------------- */

FSSR_NAMESPACE_BEGIN

inline
IsoOctree::IsoOctree (void)
{
    this->clear();
}

inline IsoOctree::VoxelVector const&
IsoOctree::get_voxels (void) const
{
    return this->voxels;
}

inline void
IsoOctree::clear (void)
{
    this->Octree::clear();
    this->voxels.clear();
    this->max_level = 19;
}

inline void
IsoOctree::set_max_level (int max_level)
{
    this->max_level = max_level;
}

inline int
IsoOctree::get_max_level (void) const
{
    return this->max_level;
}

FSSR_NAMESPACE_END

#endif /* FSSR_ISO_OCTREE_HEADER */
