/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef FSSR_OCTREE_HEADER
#define FSSR_OCTREE_HEADER

#include <vector>
#include <string>
#include <cstdint>

#include "math/vector.h"
#include "mve/mesh.h"
#include "fssr/defines.h"
#include "fssr/sample.h"

FSSR_NAMESPACE_BEGIN

/**
 * A regular octree data structure (each node has zero or eight child nodes).
 * The octree is limited to 20 levels because of the way the iterator works
 * and the voxel indexing scheme (see voxel.h).
 */
class Octree
{
public:
    /**
     * Simple recursive octree node that stores samples in a vector.
     * The node is a leaf if children is null, otherwise eight children exist.
     * The node is the root node if parent is null. In FSSR, samples are
     * inserted according to scale, thus inner nodes may contain samples.
     */
    struct Node
    {
    public:
        Node (void);
        virtual ~Node (void);

    public:
        Node* children;
        Node* parent;
        int mc_index;
        std::vector<Sample> samples;
    };

    /**
     * Octree iterator that keeps track of level and path through the octree.
     * The complete path is a series of 3 bits each indicating the octant
     * from the root towards the target node. The iterator works on octrees
     * with a maximum depth of 21.
     */
    struct Iterator
    {
    public:
        Iterator (void);
        Node* first_node (void);
        Node* first_leaf (void);
        Node* next_node (void);
        Node* next_branch (void);
        Node* next_leaf (void);
        Iterator descend (int octant) const;
        Iterator descend (uint8_t level, uint64_t path) const;
        Iterator ascend (void) const;

    public:
        Node* current;
        Node* root;
        uint64_t path;
        uint8_t level;
    };

public:
    Octree (void);
    virtual ~Octree (void);

    /** Resets the octree to its initial state. */
    void clear (void);

    /** Clears all samples in all nodes. */
    void clear_samples (void);

    /**
     * Inserts all samples from the point set into the octree.
     */
    void insert_samples (SampleList const& samples);

    /**
     * Inserts a single sample into the octree. The sample scale is used to
     * determine the approriate octree level. If the sample is outside the
     * octree root, the octree is expanded. Although new samples are not
     * inserted in levels finer than the maximum level, samples can still end
     * up in finer levels due to octree expansion. Thus limit_octree_level()
     * method must be called once after all samples have been inserted.
     */
    void insert_sample (Sample const& s);

    /** Returns the number of samples in the octree. */
    std::size_t get_num_samples (void) const;

    /**
     * Returns the number of nodes in the octree.
     */
    std::size_t get_num_nodes (void) const;

    /**
     * Returns the number of levels (WARNING: traverses whole tree).
     * For an empty octree (without any nodes), this returns 0. For
     * one root node only, this returns 1, and so on.
     */
    int get_num_levels (void) const;

    /**
     * Returns octree level statistics (WARNING: traverses whole tree).
     * For an empty octree (without any nodes), the result vector is empty.
     * Otherwise the vector contains the samples per level, root being zero.
     */
    void get_samples_per_level (std::vector<std::size_t>* stats) const;

    /** Retuns center and size for the iterator node. */
    void node_center_and_size (Iterator const& iter,
        math::Vec3d* center, double* size) const;

    /** Returns the root node (read-only). */
    Node const* get_root_node (void) const;

    /** Returns the center of the root node. */
    math::Vec3d const& get_root_node_center (void) const;

    /** Returns the size of the root node. */
    double get_root_node_size (void) const;

    /** Returns an octree iterator for the root. */
    Iterator get_iterator_for_root (void) const;

    /**
     * Queries all samples that influence the given point. The actual
     * influence distance is given as factor of the sample's scale value,
     * which depends on the basis functions used.
     */
    void influence_query (math::Vec3d const& pos, double factor,
        std::vector<Sample const*>* result) const;

    /**
     * Refines the octree by subdividing all leaves.
     */
    void refine_octree (void);

    /**
     * Limits the octree to the max level. This must be called before
     * computing the implicit function or isosurface extraction.
     */
    void limit_octree_level (void);

    /**
     * Sets the maximum level on which voxels are generated.
     * The default is 20, which is the maximum allowed level (see voxel.h).
     */
    void set_max_level (int max_level);

    /**
     * Returns the maximum level on which voxels are generated.
     * The root level is 0, children are at level 1, and so on.
     */
    int get_max_level (void) const;

    /** Prints some octree statistics to the stream. */
    void print_stats (std::ostream& out);

private:
    /* Octree functions. */
    void create_children (Node* node);
    bool is_inside_octree (math::Vec3d const& pos);
    void expand_root_for_point (math::Vec3d const& pos);

    /* Octree recursive functions. */
    Node* find_node_descend (Sample const& sample, Iterator const& iter);
    Node* find_node_expand (Sample const& sample);
    int get_num_levels (Node const* node) const;
    void get_samples_per_level (std::vector<std::size_t>* stats,
        Node const* node, std::size_t level) const;
    void influence_query (math::Vec3d const& pos, double factor,
        std::vector<Sample const*>* result, Iterator const& iter,
        math::Vec3d const& parent_node_center) const;
    void limit_octree_level (Node* node, Node* parent, int level);

private:
    /* The root node with its center and side length. */
    Node* root;
    math::Vec3d root_center;
    double root_size;

    /* The number of samples and nodes in the octree. */
    std::size_t num_samples;
    std::size_t num_nodes;

    /* Limit the octree depth. Maximum level is 20 (see voxel.h). */
    int max_level;
};

/* ------------------------- Implementation ---------------------------- */

inline
Octree::Node::Node (void)
    : children(nullptr), parent(nullptr)
{
}

inline
Octree::Node::~Node (void)
{
    delete [] this->children;
}

/* -------------------------------------------------------------------- */

inline
Octree::Iterator::Iterator (void)
    : current(nullptr)
    , root(nullptr)
    , path(0)
    , level(0)
{
}

/* ---------------------------------------------------------------- */

inline
Octree::Octree (void)
    : root(nullptr)
{
    this->clear();
}

inline
Octree::~Octree (void)
{
    delete this->root;
}

inline void
Octree::clear (void)
{
    delete this->root;
    this->root = nullptr;
    this->root_size = 0.0;
    this->root_center = math::Vec3d(0.0);
    this->num_samples = 0;
    this->num_nodes = 0;
    this->max_level = 20;
}

inline void
Octree::clear_samples (void)
{
    Iterator iter = this->get_iterator_for_root();
    for (iter.first_node(); iter.current != nullptr; iter.next_node())
        iter.current->samples.clear();
    this->num_samples = 0;
}

inline std::size_t
Octree::get_num_samples (void) const
{
    return this->num_samples;
}

inline std::size_t
Octree::get_num_nodes (void) const
{
    return this->num_nodes;
}

inline int
Octree::get_num_levels (void) const
{
    return this->get_num_levels(this->root);
}

inline void
Octree::get_samples_per_level (std::vector<std::size_t>* stats) const
{
    stats->clear();
    this->get_samples_per_level(stats, this->root, 0);
}

inline Octree::Node const*
Octree::get_root_node (void) const
{
    return this->root;
}

inline math::Vec3d const&
Octree::get_root_node_center (void) const
{
    return this->root_center;
}

inline double
Octree::get_root_node_size (void) const
{
    return this->root_size;
}

inline void
Octree::influence_query (math::Vec3d const& pos, double factor,
    std::vector<Sample const*>* result) const
{
    result->resize(0);
    this->influence_query(pos, factor, result, this->get_iterator_for_root(),
        this->root_center);
}

inline void
Octree::set_max_level (int max_level)
{
    this->max_level = std::max(0, std::min(20, max_level));
}

inline int
Octree::get_max_level (void) const
{
    return this->max_level;
}

FSSR_NAMESPACE_END

#endif // FSSR_OCTREE_HEADER
