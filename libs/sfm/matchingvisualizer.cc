#include <stdexcept>

#include "mve/imagetools.h"
#include "mve/imagedrawing.h"

#include "matchingvisualizer.h"

SFM_NAMESPACE_BEGIN

namespace
{
    unsigned char color_table[12][3] =
    {
        { 255, 0, 0 }, { 0, 255, 0 }, { 0, 0, 255 },
        { 255, 255, 0 }, { 255, 0, 255 }, { 0, 255, 255 },
        { 127, 255, 0 }, { 255, 127, 0 }, { 127, 0, 255 },
        { 255, 0, 127 }, { 0, 127, 255 }, { 0, 255, 127 }
    };
}  // namespace

mve::ByteImage::Ptr
visualizer_draw_features (mve::ByteImage::ConstPtr image,
    std::vector<std::pair<int, int> > const& matches)
{
    mve::ByteImage::Ptr ret;
    if (image->channels() == 3)
    {
        ret = mve::image::desaturate<unsigned char>(image, mve::image::DESATURATE_AVERAGE);
        ret = mve::image::expand_grayscale<unsigned char>(ret);
    }
    else if (image->channels() == 1)
    {
        ret = mve::image::expand_grayscale<unsigned char>(image);
    }

    for (std::size_t i = 0; i < matches.size(); ++i)
    {
        int x = matches[i].first;
        int y = matches[i].second;
        if (x < 0 || y < 0 || x >= image->width() || y >= image->height())
        {
            std::cerr << "Warning: Feature out of bounds: "
                << x << " " << y << std::endl;
            continue;
        }
        // TODO Draw circle
        mve::image::draw_circle(*ret, x, y, 3, color_table[3]);
    }
    return ret;
}

mve::ByteImage::Ptr
visualizer_draw_matching (mve::ByteImage::ConstPtr image1,
    mve::ByteImage::ConstPtr image2,
    std::vector<std::pair<int, int> > const& matches1,
    std::vector<std::pair<int, int> > const& matches2)
{
    if (image1->channels() != 3 || image2->channels() != 3)
        throw std::invalid_argument("Only 3-channel images allowed");

    int const img1_width = image1->width();
    int const img1_height = image1->height();
    int const img2_width = image2->width();
    int const img2_height = image2->height();
    int const out_width = img1_width + img2_width;
    int const out_height = std::max(img1_height, img2_height);
    mve::ByteImage::Ptr ret = mve::ByteImage::create(out_width, out_height, 3);
    ret->fill(0);

    /* Copy images into output image. */
    unsigned char* out_ptr = ret->begin();
    unsigned char const* img1_ptr = image1->begin();
    unsigned char const* img2_ptr = image2->begin();
    for (int y = 0; y < out_height; ++y)
    {
        if (y < img1_height)
        {
            std::copy(img1_ptr, img1_ptr + img1_width * 3, out_ptr);
            img1_ptr += img1_width * 3;
        }
        out_ptr += img1_width * 3;
        if (y < img2_height)
        {
            std::copy(img2_ptr, img2_ptr + img2_width * 3, out_ptr);
            img2_ptr += img2_width * 3;
        }
        out_ptr += img2_width * 3;
    }

    /* Draw matches. */
    for (std::size_t i = 0; i < matches1.size() && i < matches2.size(); ++i)
    {
        mve::image::draw_line(*ret, matches1[i].first, matches1[i].second,
            matches2[i].first + img1_width, matches2[i].second,
            color_table[i % 12]);
    }

    return ret;
}

SFM_NAMESPACE_END
