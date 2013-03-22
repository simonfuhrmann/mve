#include <iostream>
#include <vector>

#include "util/string.h"
#include "util/timer.h"
#include "math/vector.h"
#include "math/matrix.h"
#include "mve/defines.h"
#include "mve/trianglemesh.h"
#include "mve/meshtools.h"
#include "mve/marchingtets.h"
#include "mve/marchingcubes.h"
#include "mve/volume.h"

template <typename T>
inline T
marschner_lobb (T const& x, T const& y, T const& z)
{
    T const FM = T(6);
    T const ALPHA = T(0.25);
    T r = std::sqrt(x*x + y*y);
    return (std::sin(MATH_PI * z * T(0.5))
        + ALPHA * (T(1) + std::cos(T(2) * MATH_PI * FM
        * std::cos(MATH_PI * r * T(0.5))))) / (T(2) * (T(1) + ALPHA));
}


int main (int argc, char** argv)
{
#   define VOL_SIZE 128
#   define VOL_SIZE_2 64.0f
#   define SPHERE_RADIUS 60.0f

    mve::FloatVolume::Ptr vol(mve::FloatVolume::create(VOL_SIZE, VOL_SIZE, VOL_SIZE));
    mve::FloatVolume::Voxels& vd(vol->get_data());

    /* Populate SDF. */
    std::cout << "Generating volume..." << std::endl;
    std::size_t i = 0;
    for (std::size_t z = 0; z < VOL_SIZE; ++z)
        for (std::size_t y = 0; y < VOL_SIZE; ++y)
            for (std::size_t x = 0; x < VOL_SIZE; ++x, ++i)
            {
                math::Vec3f p((float)x - VOL_SIZE_2, (float)y - VOL_SIZE_2, (float)z - VOL_SIZE_2);
#if 1
                vd[i] = p.norm() - SPHERE_RADIUS;
#else
                p /= VOL_SIZE_2;
                vd[i] = marschner_lobb<float>(p[0], p[1], p[2]);
#endif
            }

    /* Triangulate volume. */
    std::cout << "Triangulating volume..." << std::endl;

    util::ClockTimer timer;
    mve::VolumeMTAccessor accessor;
    accessor.vol = vol;
    mve::TriangleMesh::Ptr mesh = mve::geom::marching_tetrahedra(accessor);
    std::cout << "Maching Tetrahedra took " << timer.get_elapsed() << "ms" << std::endl;
    mve::geom::save_mesh(mesh, "/tmp/mt_mesh.off");

    /* Maching cubes... */
    timer.reset();
    mve::VolumeMCAccessor mc_accessor;
    mc_accessor.vol = vol;
    mesh = mve::geom::marching_cubes(mc_accessor);
    std::cout << "Maching Cubes took " << timer.get_elapsed() << "ms" << std::endl;
    mve::geom::save_mesh(mesh, "/tmp/mc_mesh.off");

    return 0;
}
