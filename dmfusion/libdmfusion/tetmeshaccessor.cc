#include "tetmeshaccessor.h"

DMFUSION_NAMESPACE_BEGIN

bool
TetmeshAccessor::next (void)
{
    iter += 4;
    if (iter >= tets.size())
    {
        iter = static_cast<std::size_t>(-4);
        return false;
    }

    for (int i = 0; i < 4; ++i)
    {
        this->sdf[i] = this->sdf_values[this->tets[iter + i]];
        this->pos[i] = this->verts[this->tets[iter + i]];
        this->vid[i] = this->tets[iter + i];
        if (this->use_color)
            this->color[i] = math::Vec3f(*this->colors[this->tets[iter + i]]);
    }

    return true;
}

DMFUSION_NAMESPACE_END
