#ifndef DMFUSION_STANFORD_HEADER
#define DMFUSION_STANFORD_HEADER

#include <string>
#include <vector>

#include "math/vector.h"
#include "math/quaternion.h"
#include "mve/mesh_tools.h"
#include "mve/image.h"

#include "defines.h"

DMFUSION_NAMESPACE_BEGIN

struct StanfordRangeImage
{
    std::string filename;
    std::string fullpath;
    math::Vec3f translation;
    math::Quat4f rotation;
    math::Vec3f campos;
    math::Vec3f viewdir; // test
};

/* ---------------------------------------------------------------- */

class StanfordDataset
{
public:
    typedef std::vector<StanfordRangeImage> RangeImages;

private:
    RangeImages images;

public:
    StanfordDataset (void);

    void read_config (std::string const& conffile);
    RangeImages const& get_range_images (void) const;
    mve::TriangleMesh::Ptr get_mesh (StanfordRangeImage const& ri);
    mve::FloatImage::Ptr get_depth_image (StanfordRangeImage const& ri);
};

/* ---------------------------------------------------------------- */

inline
StanfordDataset::StanfordDataset (void)
{
}

inline StanfordDataset::RangeImages const&
StanfordDataset::get_range_images (void) const
{
    return this->images;
}

DMFUSION_NAMESPACE_END

#endif /* DMFUSION_STANFORD_HEADER */
