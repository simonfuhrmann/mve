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

class Octree
{
public:
    /**
     * Simple recursive octree node that stores samples in a vector.
     */
    struct Node
    {
    public:
        Node (void);
        virtual ~Node (void);

    public:
        Node* children[8];
        std::vector<Sample> samples;
    };

    /**
     * Keeps track of the current node path during descend and ascend.
     * The complete path is a series of 3 bits each indicating the octant
     * from the root towards the target node.
     */
    struct NodePath
    {
    public:
        NodePath (void);
        NodePath descend (int const octant) const;
        NodePath ascend (void) const;

    public:
        uint64_t path;
        uint8_t level;
    };

    /**
     * Keeps track of the node geometry, i.e. the node's center and size,
     * during octree descend. Support for computing the AABB of the node.
     */
    struct NodeGeom
    {
    public:
        NodeGeom (void);
        NodeGeom descend (int const octant) const;

        math::Vec3d aabb_min (void) const;
        math::Vec3d aabb_max (void) const;
        math::Vec3d corner_pos (int const corner) const;

    public:
        math::Vec3d center;
        double size;
    };

    /**
     * Comfortable iterator for the octree.
     */
    struct Iterator
    {
        Octree::Node const* node;
        Octree::NodeGeom node_geom;
        Octree::NodePath node_path;

        Iterator (void);
        Iterator (Octree const* octree);
        void init (Octree const* octree);
        Iterator descend (int const octant) const;
    };

public:
    Octree (void);
    virtual ~Octree (void);
    void clear (void);

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
     * For an octree with root node only, the vector contains one element.
     */
    void get_points_per_level (std::vector<std::size_t>* stats) const;

    /** Returns the root node (read-only). */
    Node const* get_root_node (void) const;

    /** Returns the center of the root node. */
    math::Vec3d const& get_root_node_center (void) const;

    /** Returns the size of the root node. */
    double get_root_node_size (void) const;

    /** Returns a NodeGeom object for the root. */
    NodeGeom get_node_geom_for_root (void) const;

    /** Returns a NodePath object for the root. */
    NodePath get_node_path_for_root (void) const;

    /** Returns an octree iterator for the root. */
    Iterator get_iterator_for_root (void) const;

    /**
     * Queries all samples that influence the given point. The actual
     * influence distance is given as factor of the sample's scale value.
     */
    void influence_query (math::Vec3d const& pos, double factor,
        std::vector<Sample const*>* result) const;

    /**
     * Queries all nodes that are influenced by the given sample.
     * The result is an approximation, i.e. some nodes may not actually
     * be in the support range of the sample.
     */
    void influenced_query (Sample const& sample, double factor,
        std::vector<Iterator>* result);

    /**
     * Makes the octree regular such that that every node has either
     * exactly eight (inner node) or none (leaf node) children allocated.
     */
    void make_regular_octree (void);

    /**
     * Refines the octree by subdividing all leaves.
     */
    void refine_octree (void);

    /**
     * Removes low-res samples up to a specific level.
     */
    void remove_low_res_samples (int min_level);

    /**
     * Writes the octree hierarchy to stream.
     * This does NOT write the sample data, just the hierarchy.
     */
    void write_hierarchy (std::ostream& out, bool with_meta = true) const;

    /**
     * Reads the hierarchy from stream and builds the octree.
     * This des NOT read the sample data, just the hierarchy.
     */
    void read_hierarchy (std::istream& in, bool with_meta = true);

    /** Write an octree visualization to mesh. */
    void octree_to_mesh (std::string const& filename);

    /** Computes a new center for a child specified by octant. */
    static math::Vec3d child_center_for_octant (math::Vec3d const& old_center,
        double old_size, int octant);

    /** Prints some octree statitics to the stream. */
    void print_stats (std::ostream& out);

private:
    /* Octree functions. */
    bool is_inside_octree (math::Vec3d const& pos);
    void expand_root_for_point (math::Vec3d const& pos);
    Node* find_node_for_sample (Sample const& sample);

    /* Octree recursive functions. */
    Node* find_node_descend (Sample const& sample, Node* node,
        NodeGeom const& node_geom);
    Node* find_node_expand (Sample const& sample);
    int get_num_levels (Node const* node) const;
    void get_points_per_level (std::vector<std::size_t>* stats,
        Node const* node, std::size_t level) const;
    void influence_query (math::Vec3d const& pos, double factor,
        std::vector<Sample const*>* result,
        Node const* node, NodeGeom const& node_geom) const;
    void influenced_query (Sample const& sample, double factor,
        std::vector<Iterator>* result, Iterator const& iter);
    void make_regular_octree (Node* node);

    /* Debugging functions. */
    void octree_to_mesh (mve::TriangleMesh::Ptr mesh,
        Octree::Node const* node, NodeGeom const& node_geom);

private:
    /* The number of samples in the octree. */
    std::size_t num_samples;
    /* The number of nodes in the octree (inner nodes plus leafs). */
    std::size_t num_nodes;

    /* The root node with its center and side length. */
    Node* root;
    math::Vec3d root_center;
    double root_size;
};

/* ------------------------- Implementation ---------------------------- */

inline
Octree::Node::Node (void)
{
    std::fill(this->children, this->children + 8, (Octree::Node*)NULL);
}

inline
Octree::Node::~Node (void)
{
    for (int i = 0; i < 8; ++i)
        delete this->children[i];
}

/* ---------------------------------------------------------------- */

inline
Octree::NodePath::NodePath (void)
    : path(0), level(0)
{
}

/* ---------------------------------------------------------------- */

inline
Octree::NodeGeom::NodeGeom (void)
    : center(0.0), size(0.0)
{
}

/* ---------------------------------------------------------------- */

inline
Octree::Iterator::Iterator (void)
{
}

inline
Octree::Iterator::Iterator (Octree const* octree)
{
    this->init(octree);
}

/* ---------------------------------------------------------------- */

inline
Octree::Octree (void)
{
    this->clear();
}

inline
Octree::~Octree (void)
{
    delete this->root;
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
Octree::get_points_per_level (std::vector<std::size_t>* stats) const
{
    stats->clear();
    this->get_points_per_level(stats, this->root, 0);
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
    this->influence_query(pos, factor, result, this->root,
        this->get_node_geom_for_root());
}

inline void
Octree::influenced_query (Sample const& sample, double factor,
    std::vector<Iterator>* result)
{
    result->resize(0);
    this->influenced_query(sample, factor, result,
        this->get_iterator_for_root());
}

inline void
Octree::make_regular_octree (void)
{
    this->make_regular_octree(this->root);
}

inline Octree::NodeGeom
Octree::get_node_geom_for_root (void) const
{
    NodeGeom geom;
    geom.size = this->get_root_node_size();
    geom.center = this->get_root_node_center();
    return geom;
}

inline Octree::NodePath
Octree::get_node_path_for_root (void) const
{
    return NodePath();
}

inline Octree::Iterator
Octree::get_iterator_for_root (void) const
{
    return Iterator(this);
}

FSSR_NAMESPACE_END

#endif // FSSR_OCTREE_HEADER
