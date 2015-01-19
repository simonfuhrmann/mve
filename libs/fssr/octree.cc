/*
 * This file is part of the Floating Scale Surface Reconstruction software.
 * Written by Simon Fuhrmann.
 */

#include <list>
#include <iostream>
#include <algorithm>

#include "util/timer.h"
#include "mve/mesh_io.h"
#include "fssr/octree.h"

FSSR_NAMESPACE_BEGIN

Octree::NodePath
Octree::NodePath::descend (int const octant) const
{
    NodePath path;
    path.level = this->level + 1;
    path.path = (this->path << 3) | octant;
    return path;
}

Octree::NodePath
Octree::NodePath::ascend (void) const
{
    NodePath path;
    path.level = this->level - 1;
    path.path = this->path >> 3;
    return path;
}

/* -------------------------------------------------------------------- */

Octree::NodeGeom
Octree::NodeGeom::descend (int const octant) const
{
    NodeGeom node;
    double const offset = this->size / 4.0;
    for (int i = 0; i < 3; ++i)
        node.center[i] = this->center[i]
            + ((octant & (1 << i)) ? offset : -offset);
    node.size = this->size / 2.0;
    return node;
}

math::Vec3d
Octree::NodeGeom::aabb_min (void) const
{
    return this->center - this->size / 2.0;
}

math::Vec3d
Octree::NodeGeom::aabb_max (void) const
{
    return this->center + this->size / 2.0;
}

math::Vec3d
Octree::NodeGeom::corner_pos (int const corner) const
{
    math::Vec3d pos;
    double const hs = this->size / 2.0;
    for (int i = 0; i < 3; ++i)
        pos[i] = this->center[i] + ((corner & (1 << i)) ? hs : -hs);
    return pos;
}

/* ---------------------------------------------------------------- */

void
Octree::Iterator::init (Octree const* octree)
{
    this->node = octree->get_root_node();
    this->node_geom = octree->get_node_geom_for_root();
    this->node_path = octree->get_node_path_for_root();
}

Octree::Iterator
Octree::Iterator::descend (int const octant) const
{
    Iterator iter;
    iter.node = this->node->children + octant;
    iter.node_geom = this->node_geom.descend(octant);
    iter.node_path = this->node_path.descend(octant);
    return iter;
}

/* -------------------------------------------------------------------- */

void
Octree::insert_sample (Sample const& s)
{
    if (this->root == NULL)
    {
        this->root = new Node();
        this->root_center = s.pos;
        this->root_size = s.scale;
        this->num_nodes = 1;
    }

    while (!this->is_inside_octree(s.pos))
        this->expand_root_for_point(s.pos);

    Node* node = this->find_node_for_sample(s);
    if (node == NULL)
        throw std::runtime_error("insert_sample(): No node for sample!");

    node->samples.push_back(s);
    this->num_samples += 1;
}

void
Octree::insert_samples (PointSet const& pset)
{
    PointSet::SampleList const& samples = pset.get_samples();
    for (std::size_t i = 0; i < samples.size(); i++)
        this->insert_sample(samples[i]);
}

void
Octree::create_children (Node* node)
{
    if (node->children != NULL)
        throw std::runtime_error("create_children(): Children exist!");
    node->children = new Node[8];
    this->num_nodes += 8;
    for (int i = 0; i < 8; ++i)
        node->children[i].parent = node;
}

bool
Octree::is_inside_octree (math::Vec3d const& pos)
{
    double const len2 = this->root_size / 2.0;
    for (int i = 0; i < 3; ++i)
        if (pos[i] < this->root_center[i] - len2
            || pos[i] > this->root_center[i] + len2)
            return false;
    return true;
}

void
Octree::expand_root_for_point (math::Vec3d const& pos)
{
    /* Compute old root octant and new root center and size. */
    int octant = 0;
    for (int i = 0; i < 3; ++i)
        if (pos[i] > this->root_center[i])
        {
            this->root_center[i] += this->root_size / 2.0;
        }
        else
        {
            octant |= (1 << i);
            this->root_center[i] -= this->root_size / 2.0;
        }
    this->root_size *= 2.0;

    /* Create new root. */
    Node* new_root = new Node();
    this->create_children(new_root);
    std::swap(new_root->children[octant].children, this->root->children);
    std::swap(new_root->children[octant].samples, this->root->samples);
    delete this->root;
    this->root = new_root;

    /* Fix parent pointers of old child nodes. */
    if (this->root->children[octant].children != NULL)
    {
        Node* children = this->root->children[octant].children;
        for (int i = 0; i < 8; ++i)
            children[i].parent = this->root->children + octant;
    }
}

Octree::Node*
Octree::find_node_for_sample (Sample const& sample)
{
    /*
     * Determine whether to expand the root or descend the tree.
     * A fitting level for the sample with scale s is level l with
     *
     *     scale(l) <= s < scale(l - 1)  <==>  scale(l) <= s < scale(l) * 2
     *
     * Thus, the root needs to be expanded if s >= scale(l) * 2.
     */
    if (sample.scale >= this->root_size * 2.0)
        return find_node_expand(sample);

    return this->find_node_descend(sample, this->root,
        this->get_node_geom_for_root());
}

Octree::Node*
Octree::find_node_descend (Sample const& sample, Node* node,
    NodeGeom const& node_geom)
{
    if (sample.scale > node_geom.size * 2.0)
        throw std::runtime_error("find_node_descend(): Sanity check failed!");

    /*
     * The current level l is appropriate if sample scale s is
     * scale(l) <= s < scale(l) * 2. As a sanity check, this function
     * must not be called if s >= scale(l) * 2. Otherwise descend.
     */
    if (node_geom.size <= sample.scale)
        return node;

    /* Find octant to descend. */
    int octant = 0;
    for (int i = 0; i < 3; ++i)
        if (sample.pos[i] > node_geom.center[i])
            octant |= (1 << i);

    if (node->children == NULL)
        this->create_children(node);

    return this->find_node_descend(sample, node->children + octant,
        node_geom.descend(octant));
}

Octree::Node*
Octree::find_node_expand (Sample const& sample)
{
    if (this->root_size > sample.scale)
        throw std::runtime_error("find_node_expand(): Sanity check failed!");

    /*
     * The current level l is appropriate if sample scale s is
     * scale(l) <= scale < scale(l) * 2. As a sanity check, this function
     * must not be called if scale(l) > s. Otherwise expand.
     */
    if (sample.scale < this->root_size * 2.0)
        return this->root;

    this->expand_root_for_point(sample.pos);
    return this->find_node_expand(sample);
}

int
Octree::get_num_levels (Node const* node) const
{
    if (node == NULL)
        return 0;
    if (node->children == NULL)
        return 1;
    int depth = 0;
    for (int i = 0; i < 8; ++i)
        depth = std::max(depth, this->get_num_levels(node->children + i));
    return depth + 1;
}

void
Octree::get_samples_per_level (std::vector<std::size_t>* stats,
    Node const* node, std::size_t level) const
{
    if (node == NULL)
        return;
    if (stats->size() <= level)
        stats->resize(level + 1, 0);
    stats->at(level) += node->samples.size();

    /* Descend into octree. */
    if (node->children == NULL)
        return;
    for (int i = 0; i < 8; ++i)
        this->get_samples_per_level(stats, node->children + i, level + 1);
}

void
Octree::influence_query (math::Vec3d const& pos, double factor,
    std::vector<Sample const*>* result,
    Node const* node, NodeGeom const& node_geom) const
{
    if (node == NULL)
        return;

    /*
     * Strategy is the following: Try to rule out this octree node. Assume
     * the largest scale sample (node_size * 2) in this node and compute
     * an estimate for the closest possible distance of any sample in the
     * node to the query. If 'factor' times the largest scale is less than
     * the closest distance, the node can be skipped and traversal stops.
     * Otherwise all samples in the node have to be tested.
     */

    /* Estimate for the minimum distance. No sample is closer to pos. */
    double const min_distance = (pos - node_geom.center).norm()
        - MATH_SQRT3 * node_geom.size / 2.0;
    double const max_scale = node_geom.size * 2.0;
    if (min_distance > max_scale * factor)
        return;

    /* Node could not be ruled out. Test all samples. */
    for (std::size_t i = 0; i < node->samples.size(); ++i)
    {
        Sample const& s = node->samples[i];
        if ((pos - s.pos).square_norm() > MATH_POW2(factor * s.scale))
            continue;
        result->push_back(&s);
    }

    /* Descend into octree. */
    if (node->children == NULL)
        return;
    for (int i = 0; i < 8; ++i)
    {
        this->influence_query(pos, factor, result, node->children + i,
            node_geom.descend(i));
    }
}

void
Octree::refine_octree (void)
{
    if (this->root == NULL)
        return;

    std::list<Node*> queue;
    queue.push_back(this->root);
    while (!queue.empty())
    {
        Node* node = queue.front();
        queue.pop_front();

        if (node->children == NULL)
            this->create_children(node);
        else
            for (int i = 0; i < 8; ++i)
                queue.push_back(node->children + i);
    }
}

void
Octree::print_stats (std::ostream& out)
{
    out << "Octree contains " << this->get_num_samples()
        << " samples in " << this->get_num_nodes() << " nodes on "
        << this->get_num_levels() << " levels." << std::endl;

    std::vector<std::size_t> octree_stats;
    this->get_samples_per_level(&octree_stats);

    bool printed = false;
    for (std::size_t i = 0; i < octree_stats.size(); ++i)
    {
        if (!printed && octree_stats[i] == 0)
            continue;
        else
        {
            out << "  Level " << i << ": "
                << octree_stats[i] << " samples" << std::endl;
            printed = true;
        }
    }
}

FSSR_NAMESPACE_END
