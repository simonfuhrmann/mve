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
    path.level = this->level +- 1;
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
    iter.node = this->node->children[octant];
    iter.node_geom = this->node_geom.descend(octant);
    iter.node_path = this->node_path.descend(octant);
    return iter;
}

/* -------------------------------------------------------------------- */

void
Octree::clear (void)
{
    this->num_samples = 0;
    this->num_nodes = 0;
    this->root = NULL;
    this->root_size = 0.0;
    this->root_center = math::Vec3d(0.0);
}

void
Octree::insert_sample (Sample const& s)
{
    if (this->root == NULL)
    {
        //std::cout << "INFO: Creating octree root node." << std::endl;
        this->root = new Node();
        this->root_center = s.pos;
        this->root_size = s.scale;
        this->num_nodes = 1;
    }

    while (!this->is_inside_octree(s.pos))
        this->expand_root_for_point(s.pos);

    Node* node = this->find_node_for_sample(s);
    if (node == NULL)
    {
        std::cerr << "Error finding node for sample "
            << this->num_samples << "!" << std::endl;
        return;
    }

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
    //std::cout << "INFO: Expanding root node." << std::endl;
    int root_octant = 0;
    for (int i = 0; i < 3; ++i)
        if (pos[i] > this->root_center[i])
        {
            this->root_center[i] += this->root_size / 2.0;
        }
        else
        {
            root_octant |= (1 << i);
            this->root_center[i] -= this->root_size / 2.0;
        }
    this->root_size *= 2.0;

    Node* new_root = new Node();
    new_root->children[root_octant] = this->root;
    this->root = new_root;
    this->num_nodes += 1;
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
    /*
     * The current level l is appropriate if sample scale s is
     * scale(l) <= s < scale(l) * 2. As a sanity check, this function
     * must not be called if s >= scale(l) * 2. Otherwise descend.
     */
    if (sample.scale > node_geom.size * 2.0)
        std::cerr << "WARNING: Sanity check S0 failed! "
            << "Root size: " << this->root_size
            << ", sample scale: " << sample.scale << std::endl;

    if (node_geom.size <= sample.scale)
        return node;

    /* Find octant to descend. */
    int octant = 0;
    for (int i = 0; i < 3; ++i)
        if (sample.pos[i] > node_geom.center[i])
            octant |= (1 << i);

    if (node->children[octant] == NULL)
    {
        node->children[octant] = new Node();
        this->num_nodes += 1;
    }

    return this->find_node_descend(sample, node->children[octant],
        node_geom.descend(octant));
}

Octree::Node*
Octree::find_node_expand (Sample const& sample)
{
    /*
     * The current level l is appropriate if sample scale s is
     * scale(l) <= scale < scale(l) * 2. As a sanity check, this function
     * must not be called if scale(l) > s. Otherwise expand.
     */

    //std::cout << "INFO: Find node expanding..." << std::endl;
    if (this->root_size > sample.scale)
        std::cerr << "WARNING: Sanity check S1 failed! "
            << "Root size: " << this->root_size
            << ", sample scale: " << sample.scale << std::endl;

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
    int max_level = 0;
    for (int i = 0; i < 8; ++i)
        max_level = std::max(max_level,
            this->get_num_levels(node->children[i]));
    return max_level + 1;
}

void
Octree::get_points_per_level (std::vector<std::size_t>* stats,
    Node const* node, std::size_t level) const
{
    if (node == NULL)
        return;
    if (stats->size() <= level)
        stats->resize(level + 1, 0);
    stats->at(level) += node->samples.size();
    for (int i = 0; i < 8; ++i)
        this->get_points_per_level(stats, node->children[i], level + 1);
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
     * Otherwise the node has to be tested and all samples in the node
     * are tested.
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
    for (int i = 0; i < 8; ++i)
    {
        if (node->children[i] == NULL)
            continue;
        this->influence_query(pos, factor, result, node->children[i],
            node_geom.descend(i));
    }
}

void
Octree::influenced_query (Sample const& sample, double factor,
    std::vector<Iterator>* result, Iterator const& iter)
{
    if (iter.node == NULL)
        return;

    /*
     * Strategy: Try to rule out this octree node. The node can be skipped
     * if 'factor' times the sample scale is smaller than the minimal
     * distance estimate to the node.
     */
    double const min_distance = (sample.pos - iter.node_geom.center).norm()
        - MATH_SQRT3 * iter.node_geom.size / 2.0;
    if (factor * sample.scale < min_distance)
        return;

    /* Descend into octree. */
    bool is_leaf = true;
    for (int i = 0; i < 8; ++i)
    {
        if (iter.node->children[i] == NULL)
            continue;
        is_leaf = false;
        this->influenced_query(sample, factor, result, iter.descend(i));
    }

    /* Only leafs are considered. */
    if (!is_leaf)
        return;

    /* Add this node to the result set. */
    result->push_back(iter);
}

void
Octree::make_regular_octree (Node* node)
{
    bool is_leaf = true;
    for (int i = 0; i < 8 && is_leaf; ++i)
        if (node->children[i] != NULL)
            is_leaf = false;

    if (is_leaf)
        return;

    for (int i = 0; i < 8; ++i)
    {
        if (node->children[i] == NULL)
        {
            node->children[i] = new Node();
            this->num_nodes += 1;
        }
        else
        {
            this->make_regular_octree(node->children[i]);
        }
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

        bool is_leaf = true;
        for (int i = 0; i < 8 && is_leaf; ++i)
            if (node->children[i] != NULL)
                is_leaf = false;

        if (is_leaf)
            for (int i = 0; i < 8; ++i)
            {
                node->children[i] = new Node();
                this->num_nodes += 1;
            }
        else
            for (int i = 0; i < 8; ++i)
                if (node->children[i] != NULL)
                    queue.push_back(node->children[i]);
    }
}

void
Octree::remove_low_res_samples (int min_level)
{
    if (this->root == NULL)
        return;

    std::size_t removed_samples = 0;
    std::list<std::pair<Node*, int> > queue;
    queue.push_back(std::make_pair(this->root, 0));
    while (!queue.empty())
    {
        Node* node = queue.front().first;
        int const level = queue.front().second;
        queue.pop_front();
        if (level > min_level)
            continue;
        removed_samples += node->samples.size();
        node->samples.clear();
        for (int i = 0; i < 8; ++i)
            if (node->children[i] != NULL && level < min_level)
                queue.push_back(std::make_pair(node->children[i], level + 1));
    }

    std::cout << "Removed " << removed_samples << " samples." << std::endl;
}

void
Octree::print_stats (std::ostream& out)
{
    out << "Octree contains " << this->get_num_samples()
        << " samples in " << this->get_num_nodes() << " nodes on "
        << this->get_num_levels() << " levels." << std::endl;

    std::vector<std::size_t> octree_stats;
    this->get_points_per_level(&octree_stats);

    std::size_t index = 0;
    while (index < octree_stats.size() && octree_stats[index] == 0)
        index += 1;

    out << "Samples per level:" << std::endl;
    while (index < octree_stats.size())
    {
        out << "  Level " << index << ": "
            << octree_stats[index] << " samples" << std::endl;
        index += 1;
    }
}

#if 0 // Dead code
void
Octree::octree_to_mesh (mve::TriangleMesh::Ptr mesh,
    Node const* node, NodeGeom const& node_geom)
{
    mve::TriangleMesh::VertexList& verts = mesh->get_vertices();
    mve::TriangleMesh::ColorList& colors = mesh->get_vertex_colors();

    /* Descend into child nodes first. */
    bool is_leaf = true;
    for (int i = 0; i < 8; ++i)
    {
        if (node->children[i] == NULL)
            continue;
        is_leaf = false;
        this->octree_to_mesh(mesh, node->children[i], node_geom.descend(i));
    }

    if (!is_leaf)
        return;

    /* Generate the 8 corner coordinates. */
    math::Vec3d corners[8];
    {
        math::Vec3d aabb[2] = { node_geom.aabb_min(), node_geom.aabb_max() };
        for (int i = 0; i < 8; ++i)
            for (int j = 0; j < 3; ++j)
                corners[i][j] = aabb[(i & (1 << j)) >> j][j];
    }

    /* Draw the edges between the corners. */
    int edges[12][2] = {
        {0, 1}, {0, 2}, {1, 3}, {2, 3},
        {4, 5}, {4, 6}, {5, 7}, {6, 7},
        {0, 4}, {1, 5}, {2, 6}, {3, 7}
    };
    for (int i = 0; i < 12; ++i)
        for (float j = 0.0f; j <= 1.0f; j += 0.02f)
        {
            verts.push_back(corners[edges[i][0]] * j + corners[edges[i][1]] * (1.0f - j));
            colors.push_back(math::Vec4f(0.0f, 1.0f, 0.0f, 1.0f));
        }
}

void
Octree::octree_to_mesh (std::string const& filename)
{
    mve::TriangleMesh::Ptr mesh = mve::TriangleMesh::create();
    this->octree_to_mesh(mesh, this->root, this->get_node_geom_for_root());
    mve::geom::save_mesh(mesh, filename);
}

math::Vec3d
Octree::child_center_for_octant (math::Vec3d const& old_center,
    double old_size, int octant)
{
    math::Vec3d new_center;

    double const center_offset = old_size / 4.0;
    for (int j = 0; j < 3; ++j)
        new_center[j] = old_center[j]
            + ((octant & (1 << j)) ? center_offset : -center_offset);

    return new_center;
}
#endif

FSSR_NAMESPACE_END
