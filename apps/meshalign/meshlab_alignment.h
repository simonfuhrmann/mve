#ifndef MESHLAB_DATASET_HEADER
#define MESHLAB_DATASET_HEADER

#include <vector>

#include "mve/mesh.h"
#include "range_image.h"

class MeshlabAlignment
{
public:
    typedef std::vector<RangeImage> RangeImages;

public:
    MeshlabAlignment (void);

    void read_alignment (std::string const& filename);
    RangeImages const& get_range_images (void) const;
    mve::TriangleMesh::Ptr get_mesh (RangeImage const& ri) const;

private:
    RangeImages images;
};

/* ---------------------------------------------------------------- */

inline
MeshlabAlignment::MeshlabAlignment (void)
{
}

inline MeshlabAlignment::RangeImages const&
MeshlabAlignment::get_range_images (void) const
{
    return this->images;
}

#endif // MESHLAB_DATASET_H
