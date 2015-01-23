/*
 * This file is part of the Floating Scale Surface Reconstruction software.
 * Written by Simon Fuhrmann.
 */

#ifndef FSSR_OCTREE_HEADER
#define FSSR_OCTREE_HEADER

#include <stdint.h>  // TODO: Use <cstdint> once C++11 is standard.
#include <vector>
#include <string>

#include "math/vector.h"
#include "mve/mesh.h"
#include "fssr/defines.h"
#include "fssr/sample.h"
#include "fssr/pointset.h"

FSSR_NAMESPACE_BEGIN

/**
 * A regular octree data structure (each node has zero or eight child nodes).
 * The maximum level of this octree is limited to 20 because of the way
 * the iterator works and the voxel indexing scheme (see voxel.h).
 */
class Octree
{
public:
    /**
     * Simple recursive octree node that stores samples in a vector.
     * The node is a leaf if children is NULL, otherwise eight children exist.
     * The node is the root node if parent is NULL. In FSSR, samples are
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
     * from the root towards the target node.
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
        Iterator descend (uint64_t path, uint8_t level) const;

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
     * Inserts a single sample into the Octree.
     * The scale information is used to determine the level based on the
     * size of the root node. If the sample is outside the octree root node,
     * the octree is expanded.
     */
    void insert_sample (Sample const& s);

    /**
     * Inserts all samples from the point set into the octree.
     */
    void insert_samples (PointSet const& pset);

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

    /** Prints some octree statitics to the stream. */
    void print_stats (std::ostream& out);

private:
    /* Octree functions. */
    void create_children (Node* node);
    bool is_inside_octree (math::Vec3d const& pos);
    void expand_root_for_point (math::Vec3d const& pos);
    Node* find_node_for_sample (Sample const& sample);

    /* Octree recursive functions. */
    Node* find_node_descend (Sample const& sample, Iterator const& iter);
    Node* find_node_expand (Sample const& sample);
    int get_num_levels (Node const* node) const;
    void get_samples_per_level (std::vector<std::size_t>* stats,
        Node const* node, std::size_t level) const;
    void influence_query (math::Vec3d const& pos, double factor,
        std::vector<Sample const*>* result, Iterator const& iter) const;

private:
    /* The root node with its center and side length. */
    Node* root;
    math::Vec3d root_center;
    double root_size;

    /* The number of samples and nodes in the octree. */
    std::size_t num_samples;
    std::size_t num_nodes;
};

/* ------------------------- Implementation ---------------------------- */

inline
Octree::Node::Node (void)
    : children(NULL), parent(NULL)
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
    : current(NULL)
    , root(NULL)
    , path(0)
    , level(0)
{
}

/* ---------------------------------------------------------------- */

inline
Octree::Octree (void)
    : root(NULL)
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
    this->root = NULL;
    this->root_size = 0.0;
    this->root_center = math::Vec3d(0.0);
    this->num_samples = 0;
    this->num_nodes = 0;
}

inline void
Octree::clear_samples (void)
{
    Iterator iter = this->get_iterator_for_root();
    for (iter.first_node(); iter.current != NULL; iter.next_node())
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
    this->influence_query(pos, factor, result, this->get_iterator_for_root());
}

inline Octree::Iterator
Octree::get_iterator_for_root (void) const
{
    Iterator iter;
    iter.root = this->root;
    iter.first_node();
    return iter;
}

FSSR_NAMESPACE_END

#endif // FSSR_OCTREE_HEADER
