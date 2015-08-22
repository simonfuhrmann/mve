/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include "math/vector.h"
#include "mve/marching_tets.h"
#include "mve/marching_cubes.h"
#include "mve/volume.h"

MVE_NAMESPACE_BEGIN

VolumeMTAccessor::VolumeMTAccessor (void)
    : iter(-1)
{
}

/* ---------------------------------------------------------------- */

bool
VolumeMTAccessor::next (void)
{
    std::size_t const width = static_cast<std::size_t>(this->vol->width());
    std::size_t const height = static_cast<std::size_t>(this->vol->height());
    std::size_t const depth = static_cast<std::size_t>(this->vol->depth());

    this->iter += 1;
    if (this->iter == (width - 1) * (height - 1) * (depth - 1) * 6)
        return false;

    int tet_id = static_cast<int>(this->iter % 6);
    if (tet_id == 0)
        this->load_new_cube();

    for (int i = 0; i < 4; ++i)
    {
        int vertexid = mve::geom::mt_freudenthal[tet_id][i];
        this->vid[i] = this->cube_vids[vertexid];
        this->sdf[i] = this->vol->get_data()[this->vid[i]];
        this->pos[i] = this->cube_pos[vertexid];
    }

    return true;
}

/* ---------------------------------------------------------------- */

void
VolumeMTAccessor::load_new_cube (void)
{
    int const width = this->vol->width();
    int const height = this->vol->height();

    int const base_x = (this->iter / 6) % (width - 1);
    int const base_y = (this->iter / (6 * (width - 1))) % (height - 1);
    int const base_z = (this->iter / (6 * (width - 1) * (height - 1)));
    int const base = base_z * width * height + base_y * width + base_x;

    float spacing = 1.0f / (float)(width - 1);

    this->cube_vids[0] = base;
    this->cube_vids[1] = base + 1;
    this->cube_vids[2] = base + 1 + width * height;
    this->cube_vids[3] = base + width * height;
    this->cube_vids[4] = base + width;
    this->cube_vids[5] = base + 1 + width;
    this->cube_vids[6] = base + 1 + width + width * height;
    this->cube_vids[7] = base + width + width * height;

    /* Find 8 voxel positions. */
    math::Vec3f basepos(base_x * spacing - 0.5f, base_y * spacing - 0.5f, base_z * spacing - 0.5f);
    this->cube_pos[0] = basepos;
    this->cube_pos[1] = basepos + math::Vec3f(spacing, 0, 0);
    this->cube_pos[2] = basepos + math::Vec3f(spacing, 0, spacing);
    this->cube_pos[3] = basepos + math::Vec3f(0, 0, spacing);
    this->cube_pos[4] = basepos + math::Vec3f(0, spacing, 0);
    this->cube_pos[5] = basepos + math::Vec3f(spacing, spacing, 0);
    this->cube_pos[6] = basepos + math::Vec3f(spacing, spacing, spacing);
    this->cube_pos[7] = basepos + math::Vec3f(0, spacing, spacing);
}

/* ---------------------------------------------------------------- */

VolumeMCAccessor::VolumeMCAccessor (void)
    : iter(-1)
{
}

bool
VolumeMCAccessor::next (void)
{
    std::size_t const width = static_cast<std::size_t>(this->vol->width());
    std::size_t const height = static_cast<std::size_t>(this->vol->height());
    std::size_t const depth = static_cast<std::size_t>(this->vol->depth());

    this->iter += 1;
    if (this->iter == (width - 1) * (height - 1) * (depth - 1))
        return false;

    int const base_x = iter % (width - 1);
    int const base_y = (iter / (width - 1)) % (height - 1);
    int const base_z = iter / ((width - 1) * (height - 1));
    int const base = base_z * width * height + base_y * width + base_x;

    float spacing = 1.0f / (float)(width - 1);

    this->vid[0] = base;
    this->vid[1] = base + 1;
    this->vid[2] = base + 1 + width * height;
    this->vid[3] = base + width * height;
    this->vid[4] = base + width;
    this->vid[5] = base + 1 + width;
    this->vid[6] = base + 1 + width + width * height;
    this->vid[7] = base + width + width * height;

    for (int i = 0; i < 8; ++i)
        this->sdf[i] = this->vol->get_data()[this->vid[i]];

    /* Find 8 voxel positions. */
    math::Vec3f basepos(base_x * spacing - 0.5f,
        base_y * spacing - 0.5f, base_z * spacing - 0.5f);
    this->pos[0] = basepos;
    this->pos[1] = basepos + math::Vec3f(spacing, 0, 0);
    this->pos[2] = basepos + math::Vec3f(spacing, 0, spacing);
    this->pos[3] = basepos + math::Vec3f(0, 0, spacing);
    this->pos[4] = basepos + math::Vec3f(0, spacing, 0);
    this->pos[5] = basepos + math::Vec3f(spacing, spacing, 0);
    this->pos[6] = basepos + math::Vec3f(spacing, spacing, spacing);
    this->pos[7] = basepos + math::Vec3f(0, spacing, spacing);

    return true;
}

bool
VolumeMCAccessor::has_colors (void) const
{
    return false;
}

MVE_NAMESPACE_END
