#include <cassert>
#include <string>

#include "mve/imagetools.h"
#include "mve/imagefile.h"
#include "dmrecon/mvstools.h"

MVS_NAMESPACE_BEGIN

/** encodes conversion from rgb to srgb as a look-up table,
    i.e. srgb2lin[i] with i in [0..255] contains the corresponding
    value in [0..1] in linear space */
std::vector<float> srgb2lin;

/* ------------------------------------------------------------------ */

void
initSRGB2linear()
{
    srgb2lin.resize(256);
    /* create look-up table */
    const std::size_t linThresh = (std::size_t) (0.04045 * 255.);
    for (std::size_t col = 0; col < 256; ++col)
        if (col <= linThresh)
            srgb2lin[col] = ((float) col / 255.) / 12.92;
        else
            srgb2lin[col] = pow(((float) col / 255. + 0.055) / 1.055, 2.4);
}


/* ------------------------------------------------------------------ */

void
colAndExactDeriv(util::RefPtr<mve::ImageBase> img, PixelCoords const& imgPos,
    PixelCoords const& gradDir, Samples& color, Samples& deriv)
{
    if (srgb2lin.empty()) {
        initSRGB2linear();
    }
    std::size_t width = img->width();
    std::size_t height = img->height();
    mve::ImageType type = img->get_type();

    for (std::size_t i = 0; i < imgPos.size(); ++i)
    {
        std::size_t left = floor(imgPos[i][0]);
        std::size_t top = floor(imgPos[i][1]);
        assert(left < width-1 && top < height-1);

        float x = imgPos[i][0] - left;
        float y = imgPos[i][1] - top;

        /* data position pointer */
        std::size_t p0 = (top * width + left) * 3;
        std::size_t p1 = ((top+1) * width + left) * 3;

        /* bilinear interpolation to determine color value */
        float x0, x1, x2, x3, x4, x5;
        switch (type) {
        case mve::IMAGE_TYPE_UINT8:
        {
            util::RefPtr<mve::ByteImage> bimg(img);
            x0 = (1.f-x) * srgb2lin[bimg->at(p0  )] + x * srgb2lin[bimg->at(p0+3)];
            x1 = (1.f-x) * srgb2lin[bimg->at(p0+1)] + x * srgb2lin[bimg->at(p0+4)];
            x2 = (1.f-x) * srgb2lin[bimg->at(p0+2)] + x * srgb2lin[bimg->at(p0+5)];
            x3 = (1.f-x) * srgb2lin[bimg->at(p1  )] + x * srgb2lin[bimg->at(p1+3)];
            x4 = (1.f-x) * srgb2lin[bimg->at(p1+1)] + x * srgb2lin[bimg->at(p1+4)];
            x5 = (1.f-x) * srgb2lin[bimg->at(p1+2)] + x * srgb2lin[bimg->at(p1+5)];
            break;
        }
        case mve::IMAGE_TYPE_FLOAT:
        {
            util::RefPtr<mve::FloatImage> fimg(img);
            x0 = (1.f-x) * fimg->at(p0  ) + x * fimg->at(p0+3);
            x1 = (1.f-x) * fimg->at(p0+1) + x * fimg->at(p0+4);
            x2 = (1.f-x) * fimg->at(p0+2) + x * fimg->at(p0+5);
            x3 = (1.f-x) * fimg->at(p1  ) + x * fimg->at(p1+3);
            x4 = (1.f-x) * fimg->at(p1+1) + x * fimg->at(p1+4);
            x5 = (1.f-x) * fimg->at(p1+2) + x * fimg->at(p1+5);
            break;
        }
        default:
            throw util::Exception("Invalid image type");
        }
        color[i][0] = (1.f - y) * x0 + y * x3;
        color[i][1] = (1.f - y) * x1 + y * x4;
        color[i][2] = (1.f - y) * x2 + y * x5;
        
        /* derivative in direction gradDir -- see GRIS-G wiki for details*/
        float u = gradDir[i][0];
        float v = gradDir[i][1];
        switch (type) {
        case mve::IMAGE_TYPE_UINT8:
        {
            mve::ByteImage::Ptr bimg(img);
            for (std::size_t c = 0; c < 3; ++c) {
                deriv[i][c] =
                    u * (srgb2lin[bimg->at(p0+3)] - srgb2lin[bimg->at(p0)])
                    + v * (srgb2lin[bimg->at(p1)] - srgb2lin[bimg->at(p0)])
                    + (v * x + u * y) *
                        (srgb2lin[bimg->at(p0)] - srgb2lin[bimg->at(p0+3)]
                         - srgb2lin[bimg->at(p1)] + srgb2lin[bimg->at(p1+3)]);
                ++p0; ++p1;
            }
            break;
        }
        case mve::IMAGE_TYPE_FLOAT:
        {
            mve::FloatImage::Ptr fimg(img);
            for (std::size_t c = 0; c < 3; ++c) {
                deriv[i][c] =
                    u * (fimg->at(p0+3) - fimg->at(p0))
                    + v * (fimg->at(p1) - fimg->at(p0))
                    + (v * x + u * y) * (fimg->at(p0) - fimg->at(p0+3)
                        - fimg->at(p1) + fimg->at(p1+3));
                ++p0; ++p1;
            }
            break;
        }
        default: ;
        }
    }
}

/* ------------------------------------------------------------------ */

void getXYZColorAtPix(util::RefPtr<mve::ImageBase> img,
    std::vector<math::Vec2i> const& imgPos, Samples* color)
{
    if (srgb2lin.empty()) {
        initSRGB2linear();
    }
    std::size_t width = img->width();
    mve::ImageType type = img->get_type();
    Samples::iterator itCol = color->begin();

    switch (type) {
    case mve::IMAGE_TYPE_UINT8:
    {
        mve::ByteImage::Ptr bimg(img);
        for (std::size_t i = 0; i < imgPos.size(); ++i) {
            std::size_t idx = imgPos[i][1] * width + imgPos[i][0];
            (*itCol)[0] = srgb2lin[bimg->at(idx,0)];
            (*itCol)[1] = srgb2lin[bimg->at(idx,1)];
            (*itCol)[2] = srgb2lin[bimg->at(idx,2)];
            ++itCol;
        }
        break;
    }
    case mve::IMAGE_TYPE_FLOAT:
    {
        mve::FloatImage::Ptr fimg(img);
        for (std::size_t i = 0; i < imgPos.size(); ++i) {
            std::size_t idx = imgPos[i][1] * width + imgPos[i][0];
            (*itCol)[0] = fimg->at(idx,0);
            (*itCol)[1] = fimg->at(idx,1);
            (*itCol)[2] = fimg->at(idx,2);
            ++itCol;
        }
        break;
    }
    default:
        throw util::Exception("Invalid image type");
    }
}

/* ------------------------------------------------------------------ */

void
getXYZColorAtPos(util::RefPtr<mve::ImageBase> img, PixelCoords const& imgPos,
    Samples* color)
{
    if (srgb2lin.empty()) {
        initSRGB2linear();
    }
    std::size_t width = img->width();
    std::size_t height = img->height();
    mve::ImageType type = img->get_type();
    PixelCoords::const_iterator citPos;
    Samples::iterator itCol = color->begin();

    for (citPos = imgPos.begin(); citPos != imgPos.end(); ++citPos, ++itCol) {
        size_t i = floor((*citPos)[0]);
        size_t j = floor((*citPos)[1]);
        assert(i < width-1 && j < height-1);

        float u = (*citPos)[0] - i;
        float v = (*citPos)[1] - j;
        size_t p0 = (j * width + i) * 3;
        size_t p1 = ((j+1) * width + i) * 3;
        float x0, x1, x2, x3, x4, x5;
        switch (type) {
        case mve::IMAGE_TYPE_UINT8:
        {
            mve::ByteImage::Ptr bimg(img);
            x0 = (1.f-u) * srgb2lin[bimg->at(p0  )] + u * srgb2lin[bimg->at(p0+3)];
            x1 = (1.f-u) * srgb2lin[bimg->at(p0+1)] + u * srgb2lin[bimg->at(p0+4)];
            x2 = (1.f-u) * srgb2lin[bimg->at(p0+2)] + u * srgb2lin[bimg->at(p0+5)];
            x3 = (1.f-u) * srgb2lin[bimg->at(p1  )] + u * srgb2lin[bimg->at(p1+3)];
            x4 = (1.f-u) * srgb2lin[bimg->at(p1+1)] + u * srgb2lin[bimg->at(p1+4)];
            x5 = (1.f-u) * srgb2lin[bimg->at(p1+2)] + u * srgb2lin[bimg->at(p1+5)];
            break;
        }
        case mve::IMAGE_TYPE_FLOAT:
        {
            mve::FloatImage::Ptr fimg(img);
            x0 = (1.f-u) * fimg->at(p0  ) + u * fimg->at(p0+3);
            x1 = (1.f-u) * fimg->at(p0+1) + u * fimg->at(p0+4);
            x2 = (1.f-u) * fimg->at(p0+2) + u * fimg->at(p0+5);
            x3 = (1.f-u) * fimg->at(p1  ) + u * fimg->at(p1+3);
            x4 = (1.f-u) * fimg->at(p1+1) + u * fimg->at(p1+4);
            x5 = (1.f-u) * fimg->at(p1+2) + u * fimg->at(p1+5);
            break;
        }
        default:
            throw util::Exception("Invalid image type");
        }
        (*itCol)[0] = (1.f - v) * x0 + v * x3;
        (*itCol)[1] = (1.f - v) * x1 + v * x4;
        (*itCol)[2] = (1.f - v) * x2 + v * x5;
    }
}

MVS_NAMESPACE_END
