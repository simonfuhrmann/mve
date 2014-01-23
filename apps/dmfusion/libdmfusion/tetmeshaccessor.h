#ifndef DMFUSION_TETMESHACCESSOR_HEADER
#define DMFUSION_TETMESHACCESSOR_HEADER

#include <vector>

#include "math/vector.h"

#include "defines.h"

DMFUSION_NAMESPACE_BEGIN

struct TetmeshAccessor
{
    public:
        std::vector<float> sdf_values;
        std::vector<math::Vec3f> verts;
        std::vector<unsigned int> tets;
        std::vector<math::Vec4f> colors;
        std::size_t iter;
        bool use_color;

    public:
        float sdf[4];
        unsigned int vid[4];
        math::Vec3f pos[4];
        math::Vec3f color[4];

    public:
        TetmeshAccessor (void);
        bool next (void);
        bool has_colors (void) const;
};

/* ---------------------------------------------------------------- */

inline
TetmeshAccessor::TetmeshAccessor (void)
    : iter(static_cast<std::size_t>(-4))
    , use_color(false)
{
}

inline bool
TetmeshAccessor::has_colors (void) const
{
    return this->use_color;
}

DMFUSION_NAMESPACE_END

#endif /* DMFUSION_TETMESHACCESSOR_HEADER */
