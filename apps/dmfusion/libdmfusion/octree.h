#ifndef DMFUSION_OCTREE_HEADER
#define DMFUSION_OCTREE_HEADER

#include <map>

#include "math/vector.h"
#include "mve/image.h"
#include "mve/camera.h"
#include "mve/mesh_tools.h"

#include "defines.h"

DMFUSION_NAMESPACE_BEGIN

/**
 * Representation of a single voxel referenced within octree nodes.
 */
struct VoxelData
{
    float dist;
    float weight;
    math::Vec4f color;

    VoxelData (void);
};

/**
 * Every voxel in the tree has a uniquely defined index.
 */
struct VoxelIndex
{
    char level;
    std::size_t index;

    VoxelIndex (void);
    VoxelIndex (char level, std::size_t index);

    void factor_index (std::size_t index, std::size_t& x,
        std::size_t& y, std::size_t& z) const;
    void factor_index (std::size_t index, std::size_t* xyz) const;

    void set_index (std::size_t const* xyz);

    /** Returns index corresponding to a voxel one level deeper in the tree. */
    VoxelIndex descend (void) const;
    /** Returns index corresp. to a voxel by navigating. */
    VoxelIndex navigate (int x, int y, int z) const;

    bool is_neighbor (VoxelIndex const& other) const;

    math::Vec3f get_pos (math::Vec3f const& min_aabb, math::Vec3f const& max_aabb) const;
    math::Vec3f get_pos (math::Vec3f const& center, float halfsize) const;

    bool operator< (VoxelIndex const& other) const;
};

/**
 * Representation of a triangle that can be inserted into the octree.
 * A single triangle is represented with three vertices,
 * per-vertex colors, normals and confidence values.
 */
struct OctreeTriangle
{
    math::Vec3f v[3];
    math::Vec4f c[3];
    math::Vec3f n[3];
    float conf[3];

    bool has_colors;
    bool has_confidences;
};

/* ---------------------------------------------------------------- */

/**
 * Implicit octree implementation. Implicit means that the octree hierarchy
 * is not explicitly stored. Each stored voxel has a level and index within
 * that level, thus positions can be calculated together with the tree AABB.
 *
 * Single voxel can be requested and erased given the voxel index. For
 * applications like MC, specific voxels (at a given level) can iterated
 * using std::map::upper/lower_bound. Voxels are in lexicographical order,
 * i.e. (l1,i1) < (l2,i2)  iff  (l1 < l2) OR (l1 == l2 AND i1 < i2),
 * where "lX" is the level and "iX" is the index.
 */
class Octree
{
public:
    typedef std::map<VoxelIndex, VoxelData> VoxelMap;
    typedef VoxelMap::iterator VoxelIter;

private:
    /* Octree settings. */
    float ramp_factor;
    float safety_border;
    float sampling_rate;
    bool allow_expansion;
    int forced_level;
    int coarser_levels;

    /*
     * Extends of the octree root node. The values are considered
     * undefied if the voxel map is empty.
     */
    math::Vec3f center;
    float halfsize;

    /*
     * If octree extends should be forced to a given AABB, the following
     * boolean is true and the AABB is given in center/halfsize.
     */
    bool forced_aabb;

    /*
     * An orthographic viewing direction can be enforced.
     * By default, viewing direction is perspective.
     */
    math::Vec3f viewdir;
    bool use_orthographic;


    /*
     * The tree is not explicity stored; Voxels are instanciated
     * directly and their position is given by the VoxelIndex (level
     * and index within that level) and the octree root AABB.
     * The data structure maps VoxelIndex objects to VoxelData.
     */
    VoxelMap voxels;

private:

    /** Creates a cubic root that contains AABB min/max. */
    void create_root (math::Vec3f const& min, math::Vec3f const& max);
    /** Expands the root such that AABB min/max fits into new root. */
    void expand_root (math::Vec3f const& min, math::Vec3f const& max);

    /** Inserts a single triangle at given level with given weight. */
    void insert (OctreeTriangle const& tri, int level,
        float level_weight, math::Vec3f const& campos);

public:
    Octree (void);
    ~Octree (void);

    /* ---------------------- Octree Settings --------------------- */

    /**
     * Specifies the ramp size factor. When calculating the SDF, the
     * ramp size corresponds to the footprint of the octree level times
     * the ramp factor. Default value: 5.0f.
     */
    void set_ramp_factor (float factor);

    /**
     * Specifies the size of border around the mesh as factor with respect
     * to the actual mesh AABB size. Default value: 0.25f.
     */
    void set_safety_border (float factor);

    /**
     * Triangles are sampled at the voxels, spacing of the voxels define
     * the sampling rate of the surface. Increasing the sampling rate
     * leads to placing triangles at deeper levels for more accurate
     * sampling. Default value: 1.0f.
     */
    void set_sampling_rate (float rate);

    /**
     * Specifies if octree expansion is allowed. Expansion is enabled
     * by default and increases the Octree size if a mesh/depthmap is
     * inserted that does not fit into the octree. If octree expansion
     * is disallowed, new triangles outside the octree are rejected.
     */
    void set_allow_expansion (bool allow);

    /**
     * Forces all triangle insertions to the given level.
     * Set to '0' to don't force (default).
     */
    void set_forced_level (int level);

    /**
     * Forces the octree root to the given AABB.
     */
    void set_forced_aabb (math::Vec3f const& min, math::Vec3f const& max);

    /**
     * Sets an orthographic viewing direction for the next depth map.
     * This is useful for specific orthographic scanners.
     */
    void set_orthographic_viewdir (math::Vec3f const& viewdir);

    /**
     * Sets the number of coarser levels triangles are inserted into.
     * Setting '0' corresponds to insertion in optimal level only.
     */
    void set_coarser_levels (int num);

    /* ------------------ Inserting into the tree ----------------- */

    /**
     * Inserts a depth map into the volume.
     * The depthmap is triangulated with default parameters first.
     */
    void insert (mve::FloatImage::ConstPtr dm, mve::ByteImage::ConstPtr ci,
        mve::CameraInfo const& cam, int conf_iters = 0);

    /**
     * Inserts a range image into the volume.
     */
    void insert (mve::TriangleMesh::ConstPtr mesh, math::Vec3f const& campos);

    /** Inserts a single triangle into the volume, returns the level. */
    int insert (OctreeTriangle const& tri, math::Vec3f const& campos);

    /* -------------------- Managing the tree --------------------- */

    /** Clears all SDF values. */
    void clear (void);

    /** Returns the set of voxels. Typically not used. */
    VoxelMap const& get_voxels (void) const;
    /** Returns the set of voxels. Typically not used. */
    VoxelMap& get_voxels (void);

    /** Finds a voxel. Returns NULL if there is no such voxel. */
    VoxelData const* find_voxel (VoxelIndex const& vi) const;
    /** Finds a voxel. Returns NULL if there is no such voxel. */
    VoxelData* find_voxel (VoxelIndex const& vi);

    /** Returns the position of a voxel index, existing or not. */
    math::Vec3f voxel_pos (VoxelIndex const& vi) const;

    /** Erases a voxel from the octree. Returns true on success. */
    bool erase_voxel (VoxelIndex const& index);

    /** Returns the minimum AABB for the octree. */
    math::Vec3f aabb_min (void) const;
    /** Returns the maximum AABB for the octree. */
    math::Vec3f aabb_max (void) const;
    /** Returns the center of the root node. */
    math::Vec3f const& get_center (void) const;
    /** Returns the halfsize of the root node. */
    float get_halfsize (void) const;

    /** Returns the memory consumption of the octree. */
    std::size_t get_memory_usage (void) const;

    /** Saves the octree to file by saving voxels and indices. */
    void save_octree (std::string const& filename) const;

    /** Loads the octree from file. */
    void load_octree (std::string const& filename);

    /**
     * Returns an image with an octree slice from one level.
     * The arguments are the level, the axis ID (x=0, y=1, z=2)
     * of the orthogonal axis, and the slice ID.
     * The image has six channels: the distance, the weight,
     * and RGBA for the color field.
     */
    mve::FloatImage::Ptr get_slice (int level, int axis, std::size_t id);

    /* ------------ Preparing for surface extraction -------------- */

    /**
     * Boosts voxels below 'confidence_thres' by interpolating distance
     * values from voxels at the parent level.
     *
     * Pass large value for uncertainty_thres for noisy data, e.g.
     * for MVS data pass ~3.0, pass smaller value ~0.5 for controlled
     * data such as range scans, or pass 0.0 for perfect data.
     */
    void boost_voxels (float confidence_thres);

    /**
     * Removes unconfident voxels according to given threshold.
     */
    std::size_t remove_unconfident (float thres);

    /**
     * Removes twin voxels from the octree, i.e. voxels on all levels
     * that coincide. The deepest voxel is not removed.
     */
    std::size_t remove_twins (void);
};

DMFUSION_NAMESPACE_END

/* ------------------------ Implementation ------------------------ */

DMFUSION_NAMESPACE_BEGIN

inline
VoxelData::VoxelData (void)
     : dist(0.0f), weight(0.0f), color(0.0f)
{
}

inline
VoxelIndex::VoxelIndex (void)
    : level(0), index(0)
{
}

inline
VoxelIndex::VoxelIndex (char level, std::size_t index)
    : level(level), index(index)
{
}

inline bool
VoxelIndex::operator< (VoxelIndex const& other) const
{
    if (this->level == other.level)
        return this->index < other.index;
    return this->level < other.level;
}

inline void
VoxelIndex::set_index (std::size_t const* xyz)
{
    std::size_t dim((1 << this->level) + 1);
    this->index = xyz[0] + xyz[1] * dim + xyz[2] * dim * dim;
}

inline void
VoxelIndex::factor_index (std::size_t index, std::size_t& x,
    std::size_t& y, std::size_t& z) const
{
    std::size_t one(1);
    std::size_t dim = (one << this->level) + one;
    x = index % dim;
    y = (index / dim) % dim;
    z = (index / (dim * dim)) % dim;
}

inline void
VoxelIndex::factor_index (std::size_t index, std::size_t* xyz) const
{
    this->factor_index(index, xyz[0], xyz[1], xyz[2]);
}

inline math::Vec3f
VoxelIndex::get_pos (math::Vec3f const& min_aabb, math::Vec3f const& max_aabb) const
{
    std::size_t one(1);
    std::size_t dim = (one << this->level) + one;
    std::size_t idx[3];
    this->factor_index(this->index, idx);

    math::Vec3f size(max_aabb - min_aabb);
    for (int i = 0; i < 3; ++i)
        size[i] = min_aabb[i] + size[i] * (float)idx[i] / (float)(dim - 1);
    return size;
}

inline math::Vec3f
VoxelIndex::get_pos (math::Vec3f const& center, float halfsize) const
{
    std::size_t one(1);
    std::size_t dim = (one << this->level) + one;
    std::size_t idx[3];
    this->factor_index(this->index, idx);

    math::Vec3f size(center - halfsize);
    float fullsize = 2.0f * halfsize;
    for (int i = 0; i < 3; ++i)
        size[i] = size[i] + fullsize * ((float)idx[i] / (float)(dim - 1));
    return size;
}

/* ---------------------------------------------------------------- */

inline
Octree::Octree (void)
    : ramp_factor(5.0f)
    , safety_border(0.25f)
    , sampling_rate(1.0f)
    , allow_expansion(true)
    , forced_level(0)
    , coarser_levels(2)
    , forced_aabb(false)
    , use_orthographic(false)
{
}

inline
Octree::~Octree (void)
{
    this->clear();
}

inline void
Octree::set_ramp_factor (float factor)
{
    this->ramp_factor = factor;
}

inline void
Octree::set_safety_border (float factor)
{
    this->safety_border = factor;
}

inline void
Octree::set_sampling_rate (float rate)
{
    this->sampling_rate = rate;
}

inline void
Octree::set_allow_expansion (bool allow)
{
    this->allow_expansion = allow;
}

inline void
Octree::set_forced_level (int level)
{
    this->forced_level = level;
}

inline void
Octree::set_forced_aabb (math::Vec3f const& min, math::Vec3f const& max)
{
    this->center = (min + max) / 2.0f;
    this->halfsize = (max - min).abs_valued().maximum() * 0.5f;
    this->forced_aabb = true;
}

inline void
Octree::set_orthographic_viewdir (math::Vec3f const& viewdir)
{
    this->viewdir = viewdir;
    this->use_orthographic = true;
}

inline void
Octree::set_coarser_levels (int num)
{
    this->coarser_levels = num;
}

inline void
Octree::clear (void)
{
    this->voxels.clear();
}

inline Octree::VoxelMap const&
Octree::get_voxels (void) const
{
    return this->voxels;
}

inline Octree::VoxelMap&
Octree::get_voxels (void)
{
    return this->voxels;
}

inline math::Vec3f
Octree::aabb_min (void) const
{
    return this->center - this->halfsize;
}

inline math::Vec3f
Octree::aabb_max (void) const
{
    return this->center + this->halfsize;
}

inline math::Vec3f const&
Octree::get_center (void) const
{
    return this->center;
}

inline float
Octree::get_halfsize (void) const
{
    return this->halfsize;
}

inline math::Vec3f
Octree::voxel_pos (VoxelIndex const& vi) const
{
    return vi.get_pos(this->aabb_min(), this->aabb_max());
}

DMFUSION_NAMESPACE_END

#endif /* DMFUSION_OCTREE_HEADER */
