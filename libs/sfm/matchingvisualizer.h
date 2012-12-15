#ifndef SFM_MATCHINGVISUALIZER_HEADER
#define SFM_MATCHINGVISUALIZER_HEADER

#include <utility>
#include <vector>

#include "mve/image.h"

#include "defines.h"

SFM_NAMESPACE_BEGIN

mve::ByteImage::Ptr
visualizer_draw_features (mve::ByteImage::ConstPtr image,
    std::vector<std::pair<int, int> > const& matches);

mve::ByteImage::Ptr
visualizer_draw_matching (mve::ByteImage::ConstPtr image1,
    mve::ByteImage::ConstPtr image2,
    std::vector<std::pair<int, int> > const& matches1,
    std::vector<std::pair<int, int> > const& matches2);

SFM_NAMESPACE_END

#endif /* SFM_MATCHINGVISUALIZER_HEADER */
