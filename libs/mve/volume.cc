#include "math/vector.h"
#include "mve/marchingtets.h"
#include "mve/marchingcubes.h"
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
    std::size_t w = this->vol->width();
    std::size_t h = this->vol->height();
    std::size_t d = this->vol->depth();

    this->iter += 1;
    if (this->iter == (w - 1) * (h - 1) * (d - 1) * 6)
        return false;

    int tet_id = this->iter % 6;
    if (tet_id == 0)
        this->load_new_cube();

    for (int i = 0; i < 4; ++i)
    {
        int vertexid = mve::geom::mt_freudenthal[tet_id][i];
        this->vid[i] = this->cube_vids[vertexid];
        this->sdf[i] = this->vol->get_data()[this->vid[i]];
        this->pos[i] = this->cube_pos[vertexid];
    }

    //if (!(iter % 100000))
    //    std::cout << "Iterator: " << iter << std::endl;
    return true;
}

/* ---------------------------------------------------------------- */

void
VolumeMTAccessor::load_new_cube (void)
{
    std::size_t w = this->vol->width();
    std::size_t h = this->vol->height();

    std::size_t base_x = (iter / 6) % (w - 1);
    std::size_t base_y = (iter / (6 * (w - 1))) % (h - 1);
    std::size_t base_z = (iter / (6 * (w - 1) * (h - 1)));
    std::size_t base = base_z * w * h + base_y * w + base_x;

    float spacing = 1.0f / (float)(w - 1);

    this->cube_vids[0] = base;
    this->cube_vids[1] = base + 1;
    this->cube_vids[2] = base + 1 + w * h;
    this->cube_vids[3] = base + w * h;
    this->cube_vids[4] = base + w;
    this->cube_vids[5] = base + 1 + w;
    this->cube_vids[6] = base + 1 + w + w * h;
    this->cube_vids[7] = base + w + w * h;

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
/* ---------------------------------------------------------------- */


VolumeMCAccessor::VolumeMCAccessor (void)
    : iter(-1)
{
}

bool
VolumeMCAccessor::next (void)
{
    std::size_t w = this->vol->width();
    std::size_t h = this->vol->height();
    std::size_t d = this->vol->depth();

    this->iter += 1;
    if (this->iter == (w - 1) * (h - 1) * (d - 1))
        return false;

    std::size_t base_x = iter % (w - 1);
    std::size_t base_y = (iter / (w - 1)) % (h - 1);
    std::size_t base_z = iter / ((w - 1) * (h - 1));
    std::size_t base = base_z * w * h + base_y * w + base_x;

    float spacing = 1.0f / (float)(w - 1);

    this->vid[0] = base;
    this->vid[1] = base + 1;
    this->vid[2] = base + 1 + w * h;
    this->vid[3] = base + w * h;
    this->vid[4] = base + w;
    this->vid[5] = base + 1 + w;
    this->vid[6] = base + 1 + w + w * h;
    this->vid[7] = base + w + w * h;

    for (int i = 0; i < 8; ++i)
        this->sdf[i] = this->vol->get_data()[this->vid[i]];

    /* Find 8 voxel positions. */
    math::Vec3f basepos(base_x * spacing - 0.5f, base_y * spacing - 0.5f, base_z * spacing - 0.5f);
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

MVE_NAMESPACE_END
