#ifndef DMFUSION_STANFORD_HEADER
#define DMFUSION_STANFORD_HEADER

#include <string>
#include <vector>

#include "mve/mesh.h"
#include "mve/image.h"

#include "range_image.h"

/* ---------------------------------------------------------------- */

class StanfordDataset
{
public:
    typedef std::vector<RangeImage> RangeImages;

public:
    StanfordDataset (void);

    void read_alignment (std::string const& conffile);
    RangeImages const& get_range_images (void) const;
    mve::TriangleMesh::Ptr get_mesh (RangeImage const& ri);

private:
    RangeImages images;
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

#endif /* DMFUSION_STANFORD_HEADER */
