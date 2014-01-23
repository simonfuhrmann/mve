#ifndef DMFUSION_TREEACCESSOR_HEADER
#define DMFUSION_TREEACCESSOR_HEADER

#include <stack>
#include <limits>

#include "math/vector.h"

#include "defines.h"
#include "octree.h"

DMFUSION_NAMESPACE_BEGIN

/**
 * Marching cubes (MC) accessor for surface extraction at a single level.
 */
class OctreeMCAccessor
{
private:
    Octree::VoxelMap::const_iterator start;
    Octree::VoxelMap::const_iterator end;
    Octree::VoxelMap::const_iterator iter;

public:
    Octree const* octree;
    float min_weight;
    int at_level;
    bool use_color;

public:
    float sdf[8];
    std::size_t vid[8];
    math::Vec3f pos[8];
    math::Vec3f color[8];

public:
    OctreeMCAccessor (Octree const& octree);
    bool next (void);
    bool has_colors (void) const;
};

/* ---------------------------------------------------------------- */

/**
 * Marching tets (MT) accessor. Unused code?
 */
class OctreeMTAccessor
{
public: // Tetmesh storage
    std::vector<VoxelIndex> verts;
    std::vector<unsigned int> tets;
    std::size_t iter;

public: // Interface
    Octree const* octree;
    float min_weight;
    bool use_color;

    float sdf[4];
    unsigned int vid[4];
    math::Vec3f pos[4];
    math::Vec3f color[4];

public: // Interface
    OctreeMTAccessor (void);

    bool next (void);
    bool has_colors (void) const;

    void add_vertex (VoxelIndex const& vertex);
    void add_tet (unsigned int const* ids);
    void add_tet (unsigned int v1, unsigned int v2,
        unsigned int v3, unsigned int v4);

    void save_hull_mesh (std::string const& filename);
};


/* ------------------------- Implementation ----------------------- */

inline bool
OctreeMCAccessor::has_colors (void) const
{
    return this->use_color;
}

inline
OctreeMTAccessor::OctreeMTAccessor (void)
    : iter(std::numeric_limits<std::size_t>::max())
    , octree(0)
    , min_weight(0.0f)
    , use_color(false)
{
}

inline bool
OctreeMTAccessor::has_colors (void) const
{
    return this->use_color;
}

DMFUSION_NAMESPACE_END

#endif /* DMFUSION_TREEACCESSOR_HEADER */
