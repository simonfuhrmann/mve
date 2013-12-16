#include <iostream>
#include <stdexcept>

#include "octreeaccessor.h"

DMFUSION_NAMESPACE_BEGIN

OctreeMCAccessor::OctreeMCAccessor (Octree const& octree)
    : octree(&octree)
    , min_weight(0.0f)
    , at_level(9)
    , use_color(true)
{
    this->iter = this->octree->get_voxels().end();
}

/* ---------------------------------------------------------------- */

bool
OctreeMCAccessor::next (void)
{
    VoxelIndex start_index(this->at_level, 0);
    VoxelIndex end_index(this->at_level, std::numeric_limits<std::size_t>::max());

    Octree::VoxelMap const& vmap(this->octree->get_voxels());

    if (this->iter == vmap.end())
    {
        this->start = vmap.lower_bound(start_index);
        this->end = vmap.upper_bound(end_index);
        this->iter = this->start;
    }
    else
    {
        this->iter++;
    }

    int const mcorder[8] = { 0, 1, 5, 4, 2, 3, 7, 6 };

    for (; this->iter != this->end && this->iter != vmap.end(); ++this->iter)
    {
        VoxelIndex const& vi = this->iter->first;

        /* Skip boundary voxels. */
        std::size_t dim = (1 << vi.level);
        std::size_t xyz[3];
        vi.factor_index(vi.index, xyz);
        if (xyz[0] == dim || xyz[1] == dim || xyz[2] == dim)
            continue;

        //VoxelData const& vd = this->iter->second;

        if (vi.level != this->at_level)
            std::cout << "Warning: Unxepected voxel: " << vi.level << ", " << vi.index << std::endl;

        VoxelIndex vis[8];
        VoxelData const* vds[8];
        int cubeconfig = 0;
        bool valid = true;
        for (int i = 0; valid && i < 8; ++i)
        {
            int mci = mcorder[i];
            vis[i] = vi.navigate(mci & 1, (mci & 2) >> 1, (mci & 4) >> 2);
            vds[i] = this->octree->find_voxel(vis[i]);
            if (!vds[i] || vds[i]->weight <= this->min_weight)
            {
                valid = false;
                break;
            }
            if (vds[i]->dist < 0.0f)
                cubeconfig |= (1 << i);

            this->sdf[i] = vds[i]->dist;
            this->pos[i] = this->octree->voxel_pos(vis[i]);
            this->vid[i] = vis[i].index; // Bug? Index not unique?

            if (this->use_color)
                this->color[i] = math::Vec3f(*vds[i]->color);
                //this->color[i] = math::Vec3f(voxel->weight / 5.0f, 0.0f, 0.0f);
        }

        /* Check cube config and skip irrelevant cubes. */
        if (valid && cubeconfig != 0x0 && cubeconfig != 0xff)
            return true;
    }

    return false;
}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool
OctreeMTAccessor::next (void)
{
    if (this->iter == std::numeric_limits<std::size_t>::max())
        this->iter = 0;
    else
        this->iter += 4;

    for (; this->iter < this->tets.size(); this->iter += 4)
    {
        /* Determine cube config and provide MC values. */
        bool valid = true;
        for (int i = 0; valid && i < 4; ++i)
        {
            VoxelIndex const& voxel = this->verts[this->tets[this->iter + i]];
            VoxelData const* vdata = this->octree->find_voxel(voxel);
            if (vdata == 0 || vdata->weight <= this->min_weight)
            {
                valid = false;
                continue;
            }

            this->sdf[i] = vdata->dist;
            this->pos[i] = this->octree->voxel_pos(voxel);
            this->vid[i] = this->tets[this->iter + i];

#if 1 // real colors
            if (this->use_color)
                this->color[i] = math::Vec3f(*vdata->color);
#else // colors based on confidence / weight
            if (this->use_color)
                this->color[i] = math::Vec3f(vdata->weight / 5.0f, 0.0f, 0.0f);
#endif
        }

        if (!valid)
            continue;

        return true;
    }

    this->iter = std::numeric_limits<std::size_t>::max();
    return false;
}

/* ---------------------------------------------------------------- */

void
OctreeMTAccessor::add_vertex (VoxelIndex const& vertex)
{
    this->verts.push_back(vertex);
}

void
OctreeMTAccessor::add_tet (unsigned int v1, unsigned int v2,
        unsigned int v3, unsigned int v4)
{
    this->tets.push_back(v1);
    this->tets.push_back(v2);
    this->tets.push_back(v3);
    this->tets.push_back(v4);
}

void
OctreeMTAccessor::add_tet (unsigned int const* ids)
{
    this->tets.insert(this->tets.end(), ids, ids + 4);
}


DMFUSION_NAMESPACE_END
