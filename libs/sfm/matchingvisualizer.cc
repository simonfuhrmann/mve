#include "mve/imagetools.h"

#include "matchingvisualizer.h"

SFM_NAMESPACE_BEGIN

mve::ByteImage::Ptr
visualizer_draw_features (mve::ByteImage::ConstPtr image,
    std::vector<std::pair<int, int> > const& matches)
{
    mve::ByteImage::Ptr ret;
    if (image->channels() == 3)
    {
        ret = mve::image::desaturate(image, mve::image::DESATURATE_AVERAGE);
        ret = mve::image::expand_grayscale(ret);
    }
    else if (image->channels() == 1)
    {
        ret = mve::image::expand_grayscale(image);
    }

    for (std::size_t i = 0; i < matches.size(); ++i)
    {
        int x = matches[i].first;
        int y = matches[i].second;
        ret->at(x, y, 0) = 255;
        ret->at(x, y, 1) = 0;
        ret->at(x, y, 2) = 0;
    }
    return ret;
}

SFM_NAMESPACE_END
