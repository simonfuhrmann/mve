#include <iostream>

#include "image.h"
#include "imagetools.h"
#include "imagefile.h" // TMP

#include "surf.h"

MVE_NAMESPACE_BEGIN

/* Kernel sizes (cols) for the octaves (rows). */
int kernel[4][4] =
{
    {  3,  5,  7,  9 },
    {  5,  9, 13, 17 },
    {  9, 17, 25, 33 },
    { 17, 33, 49, 65 }
};

/* ---------------------------------------------------------------- */

void
Surf::process (void)
{
    /* Desaturate input image. */
    if (this->orig->channels() != 1 && this->orig->channels() != 3)
        throw std::invalid_argument("Gray or color image expected");
    if (this->orig->channels() == 3)
        this->orig = mve::image::desaturate<uint8_t>(this->orig, mve::image::DESATURATE_LIGHTNESS);

    /* Build summed area table (SAT). */
    this->sat = mve::image::integral_image<uint8_t,SatType>(this->orig);

    /* Compute Hessian response maps and find SS maxima (SURF 3.3). */
    this->create_octaves();

    /* Detect local extrema in the SS of Hessian response maps. */
    this->extrema_detection();
}

/* ---------------------------------------------------------------- */

void
Surf::create_octaves (void)
{
    /* Prepare octaves. */
    this->octaves.resize(4);
    for (int i = 0; i < 4; ++i)
        this->octaves[i].imgs.resize(4);

    /* Create octaves. */
    for (int o = 0; o < 4; ++o)
        for (int k = 0; k < 4; ++k)
            this->create_response_map(o, k);
}

/* ---------------------------------------------------------------- */

void
Surf::create_response_map (int o, int k)
{
    /*
     * In order to create the Hessian response map for filter size 'fs',
     * we need to convolute the image with second order Gaussian partial
     * derivative filters Dxx, Dyy and Dxy and compute the response
     * as det(H) = Dxx * Dyy - w * Dxy * Dxy, where w = 0.81.
     * So we need to compute Dxx, Dyy and Dxy with filter size 'fs'.
     * Note: filter size 'fs' is defined as filter width / 3.
     * For details, see SURF 3.2.
     */
    typedef SurfOctave::RespType RespType;

    int fs = kernel[o][k]; // filter kernel size
    int fw = fs * 3; // kernel width in pixel
    int step = math::algo::fastpow(2, o); // sub-sampling spacing
    RespType weight = 0.81; // See OpenSurf constant
    RespType inv_karea = 1.0 / (RespType)(fw * fw); // normalization

    /* Original dimensions and octave dimensions. */
    std::size_t w = this->orig->width();
    std::size_t h = this->orig->height();
    std::size_t ow = w;
    std::size_t oh = h;
    for (int i = 0; i < o; ++i)
    {
        ow = (ow + 1) >> 1;
        oh = (oh + 1) >> 1;
    }

    SurfOctave::RespImage::Ptr img = SurfOctave::RespImage::create(w, h, 1);
    mve::ByteImage::Ptr outimg = mve::ByteImage::create(ow, oh, 1); //tmp

    /* Generate the response maps. */
    std::size_t i = 0;
    for (std::size_t y = 0; y < h; y += step)
        for (std::size_t x = 0; x < w; x += step, ++i)
        {
            SatType dxx = this->filter_dxx(fs, x, y);
            SatType dyy = this->filter_dyy(fs, x, y);
            SatType dxy = this->filter_dxy(fs, x, y);
            SurfOctave::RespType dxx_t = (SurfOctave::RespType)dxx * inv_karea;
            SurfOctave::RespType dyy_t = (SurfOctave::RespType)dyy * inv_karea;
            SurfOctave::RespType dxy_t = (SurfOctave::RespType)dxy * inv_karea;

            img->at(i) = dxx_t * dyy_t - weight * dxy_t * dxy_t;
            outimg->at(i) = math::algo::clamp(img->at(i) * 1.0, -128.0, 127.0) + 128; //tmp
        }

#if 1 //tmp
    std::cout << "Saving response map " << o << "." << k << std::endl;
    std::stringstream ss;
    ss << "/tmp/response-" << o << "-" << k << ".png";
    mve::image::save_file(outimg, ss.str());
#endif

    this->octaves[o].imgs[k] = img;
}

/* ---------------------------------------------------------------- */

Surf::SatType
Surf::filter_dxx (int fs, int x, int y)
{
    int fs2 = fs / 2;
    int y1 = y - (fs - 1);
    int y2 = y + (fs - 1);
    int x1 = x - fs - fs2;
    int x2 = x - fs2;
    int x3 = x + fs2;
    int x4 = x + fs + fs2;

    int w = this->sat->width();
    int h = this->sat->height();
    if (y1 < 0 || y2 >= h || x1 < 0 || x4 >= w)
        return 0;

    Surf::SatType ret = 0;
    ret += image::integral_image_area<SatType>(this->sat, x1, y1, x2-1, y2);
    ret -= 2 * image::integral_image_area<SatType>(this->sat, x2, y1, x3, y2);
    ret += image::integral_image_area<SatType>(this->sat, x3+1, y1, x4, y2);
    return ret;
}

Surf::SatType
Surf::filter_dyy (int fs, int x, int y)
{
    int fs2 = fs / 2;
    int x1 = x - (fs - 1);
    int x2 = x + (fs - 1);
    int y1 = y - fs - fs2;
    int y2 = y - fs2;
    int y3 = y + fs2;
    int y4 = y + fs + fs2;

    int w = this->sat->width();
    int h = this->sat->height();
    if (y1 < 0 || y4 >= h || x1 < 0 || x2 >= w)
        return 0;

    Surf::SatType ret = 0;
    ret += image::integral_image_area<SatType>(this->sat, x1, y1, x2, y2-1);
    ret -= 2 * image::integral_image_area<SatType>(this->sat, x1, y2, x2, y3);
    ret += image::integral_image_area<SatType>(this->sat, x1, y3+1, x2, y4);
    return ret;
}

Surf::SatType
Surf::filter_dxy (int fs, int x, int y)
{
    int x1 = x - fs;
    int x2 = x - 1;
    int x3 = x + 1;
    int x4 = x + fs;
    int y1 = y - fs;
    int y2 = y - 1;
    int y3 = y + 1;
    int y4 = y + fs;

    int w = this->sat->width();
    int h = this->sat->height();
    if (y1 < 0 || y4 >= h || x1 < 0 || x4 >= w)
        return 0;

    Surf::SatType ret = 0;
    ret += image::integral_image_area<SatType>(this->sat, x1, y1, x2, y2);
    ret -= image::integral_image_area<SatType>(this->sat, x3, y1, x4, y2);
    ret += image::integral_image_area<SatType>(this->sat, x1, y3, x2, y4);
    ret -= image::integral_image_area<SatType>(this->sat, x3, y3, x4, y4);
    return ret;
}

/* ---------------------------------------------------------------- */

void
Surf::extrema_detection (void)
{
    for (std::size_t o = 0; o < this->octaves.size(); ++o)
        for (std::size_t s = 0; s < 1; ++s)
        {
            SurfOctave::RespImage::Ptr samples[3] =
            {
                this->octaves[o].imgs[s + 0],
                this->octaves[o].imgs[s + 1],
                this->octaves[o].imgs[s + 2]
            };
            this->extrema_detection(samples, o, s);
        }
}

/* ---------------------------------------------------------------- */

#if 1
void
Surf::extrema_detection (SurfOctave::RespImage::Ptr samples[3], int o, int s)
{
    /*
     * Compute boundary for octave and scale space sample.
     * Within this boundary, no scale space maximum is detected.
     */

    //asdf
}
#endif

MVE_NAMESPACE_END
