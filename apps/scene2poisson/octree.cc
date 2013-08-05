#include <iostream>
#include <stack>

#include "octree.h"

/* ---------------------------------------------------------------- */

void
Octree::set_aabb (math::Vec3f const& min, math::Vec3f const& max)
{
    this->center = (min + max) / 2.0f;
    this->halfsize = (max - min).maximum() / 2.0f;
}

/* ---------------------------------------------------------------- */

void
Octree::insert (Point const& p)
{
    if ((p.pos - this->center).abs_value().maximum() > this->halfsize)
    {
        //std::cout << "Warning: Point outside Octree AABB!" << std::endl;
        return;
    }

    if (this->root == NULL)
        this->root = new OctreeNode();

    /* "Footprint" of the root node... */
    float root_fp = this->halfsize * 2.0f;
    float log2 = std::log(root_fp / p.footprint) / MATH_LN2;
    int level = static_cast<int>(std::ceil(log2));

    /* Descend into octree. */
    OctreeNode* node = this->root;
    math::Vec3f node_center = this->center;
    float node_hs = this->halfsize;

    for (int i = 0; i < level; ++i)
    {
        bool oct_x = p.pos[0] > node_center[0];
        bool oct_y = p.pos[1] > node_center[1];
        bool oct_z = p.pos[2] > node_center[2];
        int octant = ((int)oct_z << 2) + ((int)oct_y << 1) + (int)oct_x;

        if (node->children[octant] == NULL)
            node->children[octant] = new OctreeNode();
        node = node->children[octant];
        node_hs /= 2.0f;
        node_center[0] += node_hs * (oct_x ? 1.0f : -1.0f);
        node_center[1] += node_hs * (oct_y ? 1.0f : -1.0f);
        node_center[2] += node_hs * (oct_z ? 1.0f : -1.0f);
    }

    /* Finally, update the octree node values. */
    float w1 = node->weight;
    float w2 = p.confidence;
    node->weight = w1 + w2;
    node->pos = (node->pos * w1 + p.pos * w2) / node->weight;
    node->color = (node->color * w1 + p.color * w2) / node->weight;
    node->normal = (node->normal * w1 + p.normal * w2).normalize();
}

/* ---------------------------------------------------------------- */

mve::TriangleMesh::Ptr
Octree::get_pointset (float thres) const
{

    //FIXME Smarter extraction, recursive with "generated" return flag

    mve::TriangleMesh::Ptr ret(mve::TriangleMesh::create());
    mve::TriangleMesh::VertexList& verts(ret->get_vertices());
    mve::TriangleMesh::NormalList& vnorm(ret->get_vertex_normals());
    mve::TriangleMesh::ColorList& vcol(ret->get_vertex_colors());
    mve::TriangleMesh::ConfidenceList& vconf(ret->get_vertex_confidences());

    std::stack<OctreeNode*> nodes;
    nodes.push(this->root);
    while (!nodes.empty())
    {
        OctreeNode* node = nodes.top();
        nodes.pop();
        if (node == NULL)
            continue;

        bool leaf = true;
        for (int i = 0; i < 8; ++i)
        {
            if (node->children[i])
            {
                nodes.push(node->children[i]);
                leaf = false;
            }
        }

        if (!leaf)
            continue;

        if (node->weight < thres)
            continue;

        verts.push_back(node->pos);
        vnorm.push_back(node->normal);
        vcol.push_back(math::Vec4f(node->color, 1.0f));
        vconf.push_back(node->weight);
    }

    return ret;
}
