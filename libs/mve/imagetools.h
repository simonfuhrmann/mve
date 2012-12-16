/*
 * Image tools: Conversions, rescaling, desaturation, ...
 * Written by Simon Fuhrmann.
 */

#ifndef MVE_IMAGE_TOOLS_HEADER
#define MVE_IMAGE_TOOLS_HEADER

#include <iostream> //RM
#include <limits>

#include "util/exception.h"
#include "math/accum.h"
#include "math/algo.h"

#include "defines.h"
#include "camera.h"
#include "image.h"

MVE_NAMESPACE_BEGIN
MVE_IMAGE_NAMESPACE_BEGIN

/*
 * ----------------------- Image conversions ---------------------
 */

/**
 * Converts a given byte image to a float image.
 * This is done by scaling from [0, 255] to [0, 1].
 */
FloatImage::Ptr
byte_to_float_image (ByteImage::ConstPtr image);

/** Converts a given byte image to a double image.
 * This is done by scaling from [0, 255] to [0, 1].
 */
DoubleImage::Ptr
byte_to_double_image (ByteImage::ConstPtr image);

/**
 * Converts a given float image to a byte image.
 * This is done by clamping values to [vmin, vmax] and scaling to [0, 255].
 */
ByteImage::Ptr
float_to_byte_image (FloatImage::ConstPtr image,
    float vmin = 0.0f, float vmax = 1.0f);

/**
 * Converts a given double image to a byte image.
 * This is done by clamping values to [vmin, vmax] and scaling to [0, 255].
 */
ByteImage::Ptr
double_to_byte_image (DoubleImage::ConstPtr image,
    double vmin = 0.0, double vmax = 1.0);

/**
 * Convertes a given int image to a byte image.
 * Conversion is done by clamping absolute values.
 */
ByteImage::Ptr
int_to_byte_image (IntImage::ConstPtr image);

/**
 * Generic conversion between image types without scaling or clamping.
 * This is useful to convert between float and double.
 */
template <typename SRC, typename DST>
typename Image<DST>::Ptr
type_to_type_image (typename Image<SRC>::ConstPtr image);

/**
 * Finds the smallest and largest value in the given image.
 */
template <typename T>
void
find_min_max_value (typename mve::Image<T>::Ptr image, T* vmin, T* vmax);

/**
 * Normalizes a float image IN-PLACE such that all values are [0, 1].
 * This is done by first finding the largest value in the image
 * and dividing all image values by the largest value.
 */
void
float_image_normalize (FloatImage::Ptr image);

/*
 * ----------------------- Image undistortion ---------------------
 */

/**
 * Undistorts the input image using the parameters given in the camera.
 * It relies on the focal length in the camera to be in unit format.
 * This is a strange distortion model used in the Photosynther.
 */
template <typename T>
typename Image<T>::Ptr
image_undistort (typename Image<T>::ConstPtr img, CameraInfo const& cam);

/**
 * Undistorts the input image using the parameters given in the camera.
 * It relies on the focal length in the camera to be in unit format.
 * If both distortion parameters are 0, the undistortion has no effect.
 * This is the distortion model used by the Noah bundler.
 */
template <typename T>
typename Image<T>::Ptr
image_undistort_noah (typename Image<T>::ConstPtr img, CameraInfo const& cam);

/*
 * ------------------- Image scaling and cropping -----------------
 */

/**
 * Returns a sub-image by cropping against a rectangular region.
 * Region may exceed the input image dimensions, new pixel values
 * are initialized with zero.
 */
template <typename T>
typename Image<T>::Ptr
crop (typename Image<T>::ConstPtr image, int left, int top,
    int width, int height);

/** Rescale interpolation type. */
enum RescaleInterpolation
{
    RESCALE_NEAREST,
    RESCALE_LINEAR,
    RESCALE_GAUSSIAN ///< Not suited for byte images
    //RESCALE_CUBIC // Not implemented
};

/**
 * Returns a rescaled version of 'image' with dimensions 'width'
 * times 'height' using 'interp' for value interpolation. Set one of
 * 'width' or 'height' to 0 to keep aspect ratio. Mipmap reduction
 * is applied if the image is rescaled with size factor < 1/2.
 */
template <typename T>
typename Image<T>::Ptr
rescale (typename Image<T>::ConstPtr image, RescaleInterpolation interp,
    int width, int height);

/**
 * Returns a rescaled version of image, scaled by factor 1/2, by grouping
 * blocks of 2x2 pixel to one pixel in the new image.
 */
template <typename T>
typename Image<T>::Ptr
rescale_half_size (typename Image<T>::ConstPtr image);

/**
 * Returns a rescaled version of the image, scaled with a gaussian
 * approximation by factor 1/2. Note that the kernel size if fixed to
 * 4x4 pixels, so sigma shouldn't be much larger than 1. The default sigma
 * is sqrt(1.0^2 - 0.5^2) =~ 0.866 to double the inherent sigma of 0.5.
 */
template <typename T>
typename Image<T>::Ptr
rescale_half_size_gaussian (typename Image<T>::ConstPtr image,
    float sigma = 0.866025403784439f);

/**
 * Returns a rescaled version of the image by subsampling every second
 * column and row. Useful if the original image already has appropriate blur.
 */
template <typename T>
typename Image<T>::Ptr
rescale_half_size_subsample (typename Image<T>::ConstPtr image);

/**
 * Returns a rescaled version of the image, upscaled with linear
 * interpolation by factor 2. In this version, only interpolated values
 * are used to construct the new image. This is eliminates some original
 * information but preserves the dimension/appearance of the image.
 */
template <typename T>
typename Image<T>::Ptr
rescale_double_size (typename Image<T>::ConstPtr img);

/**
 * Returns a rescaled version of the image, upscaled with linear
 * interpolation. Every second row and column is directly taken.
 * This technique preserves all information (i.e. the original image
 * can be recovered using rescale_half_size_subsample), but introduces
 * a half pixel shift towards the upper left corner.
 */
template <typename T>
typename Image<T>::Ptr
rescale_double_size_supersample (typename Image<T>::ConstPtr img);

/**
 * Rescales image 'in' using nearest neighbor. The new images is scaled to
 * the dimensions of 'out', placing the result in 'out'.
 * When downsampling, requires 'in' to be at appropriate mipmap level
 * to avoid aliasing artifacts.
 */
template <typename T>
void
rescale_nearest (typename Image<T>::ConstPtr in, typename Image<T>::Ptr out);

/**
 * Rescales image 'in' using linear interpolation. The new images is
 * scaled to the dimensions of 'out', placing the result in 'out'.
 * When downsampling, requires 'in' to be at appropriate mipmap level
 * to avoid aliasing artifacts.
 */
template <typename T>
void
rescale_linear (typename Image<T>::ConstPtr in, typename Image<T>::Ptr out);

/**
 * Rescales image 'in' using a gaussian kernel mask. The new image is
 * scaled to the dimension of 'out', placing the result in 'out'.
 * A smaller sigma factor produces more crisp results but aliased results,
 * whereas a larger sigma factor produces smoother but blurred results.
 * Warning: This function is terribly slow due to naive implementation.
 */
template <typename T>
void
rescale_gaussian (typename Image<T>::ConstPtr in,
    typename Image<T>::Ptr out, float sigma_factor = 1.0f);

/*
 * ------------------------- Image blurring --------------------------
 */

/**
 * Blurs the image using a gaussian convolution kernel.
 * The implementation exploits kernel separability.
 */
template <typename T>
typename Image<T>::Ptr
blur_gaussian (typename Image<T>::ConstPtr in, float sigma);

/**
 * Blurs the image using a box filter of integer size 'ks'.
 * The implementaion is separated, and much faster than Gaussian blur,
 * but yields horizontal and vertical artifacts.
 */
template <typename T>
typename Image<T>::Ptr
blur_boxfilter (typename Image<T>::ConstPtr in, int ks);

/*
 * ------------------- Image rotation and flipping -------------------
 */

/** Image rotation type. */
enum RotateType
{
    ROTATE_CCW, ///< Counter-clock wise rotation
    ROTATE_CW, ///< Clock wise rotation
    ROTATE_180, ///< 180 degree rotation
    ROTATE_SWAP ///< Exchanges x- and y-axis
};

/**
 * Returns a rotated copy of the given image either rotated clock wise,
 * counter-clock wise, with 180 degree or with swapped x- and y-axis.
 * Swapping x- and y-axis is not a real rotation but rather transposing.
 * Note: In-place not supported because CW, CCW rotation and swap changes
 * image dimensions. An in-place version of ROTATE_180 is available using
 * in-place flipping in both directions.
 */
template <typename T>
typename Image<T>::Ptr
rotate (typename Image<T>::ConstPtr image, RotateType type);

/** Image flipping type. */
enum FlipType
{
    FLIP_NONE = 0,
    FLIP_HORIZONTAL = 1 << 0,
    FLIP_VERTICAL = 1 << 1,
    FLIP_BOTH = FLIP_HORIZONTAL | FLIP_VERTICAL
};

/**
 * Flips the given image either horizontally, vertically or both IN-PLACE.
 */
template <typename T>
void
flip (typename Image<T>::Ptr image, FlipType type);

/*
 * ----------------------- Image desaturation ------------------------
 */

/** Desaturaturation type. */
enum DesaturateType
{
    DESATURATE_MAXIMUM, ///< max(R,G,B)
    DESATURATE_LIGHTNESS, ///< (max(R,G,B) + min(R,G,B)) * 1/2
    DESATURATE_LUMINOSITY, ///< 0.21 * R + 0.72 * G + 0.07 * B
    DESATURATE_LUMINANCE, ///< 0.30 * R + 0.59 * G + 0.11 * B
    DESATURATE_AVERAGE ///< (R + G + B) * 1/3
};

/**
 * Desaturates an RGB or RGBA image to G or GA respectively.
 * A new image is returned, the original image is untouched.
 *
 * From http://en.wikipedia.org/wiki/HSL_and_HSV#Lightness
 *
 * Maximum = max(R,G,B)
 * Lightness = 1/2 * (max(R,G,B) + min(R,G,B))
 * Luminosity = 0.21 * R + 0.72 * G + 0.07 * B
 * Luminance = 0.30 * R + 0.59 * G + 0.11 * B
 * Average Brightness = 1/3 * (R + G + B)
 */
template <typename T>
typename Image<T>::Ptr
desaturate (typename Image<T>::ConstPtr image, DesaturateType type);

/**
 * Expands a gray image (one or two channels) to an RGB or RGBA image.
 */
template <typename T>
typename Image<T>::Ptr
expand_grayscale (typename Image<T>::ConstPtr image);

/** Reduce alpha: Reduces RGBA or GA images to RGB or G images. */
template <typename T>
void
reduce_alpha (typename mve::Image<T>::Ptr img); // TODO Blend color?

/* ------------------------- Edge detection ----------------------- */

/**
 * Implementation of the Sobel operator.
 * For details, see http://en.wikipedia.org/wiki/Sobel_operator
 * For byte images, the operation can lead to clipped values.
 * Likewise for floating point images, it leads to values >1.
 */
template <typename T>
typename mve::Image<T>::Ptr
sobel_edge (typename mve::Image<T>::ConstPtr img);

/* ------------------------- Miscellaneous ------------------------ */

/**
 * Subtracts two images to create the signed difference between the values.
 * This does not work for unsigned image types, use image difference instead.
 */
template <typename T>
typename Image<T>::Ptr
subtract (typename Image<T>::Ptr i1, typename Image<T>::Ptr i2);

/**
 * Creates a difference image by computing the absolute difference per value.
 * This works for unsigned image types but discards the sign.
 * The function requires 'operator<' to be defined on the image value type.
 */
template <typename T>
typename Image<T>::Ptr
difference (typename Image<T>::Ptr i1, typename Image<T>::Ptr i2);

/**
 * Applies gamma correction to float/double images (in-place).
 * To get from intensities to color values, use 1.0/2.2 as exponent.
 * To remove gamma correction from an image, use 2.2 as exponent.
 */
template <typename T>
void
gamma_correct (typename Image<T>::Ptr image, T const& power);

/**
 * Applies fast gamma correction to byte image using a lookup table.
 * Note that alpha channels are not treated as such and are also corrected!
 */
void
gamma_correct (ByteImage::Ptr image, float power);

/**
 * Calculates the integral image (or summed area table) for the input image.
 * The integral image is computed channel-wise, i.e. the output image has
 * the same amount of channels as the input image.
 */
template <typename IN, typename OUT>
typename Image<OUT>::Ptr
integral_image (typename Image<IN>::ConstPtr image);

/**
 * Sums over the rectangle defined by A=(x1,y1) and B=(x2,y2) on the given
 * SAT for channel cc. This is efficiently calculated as B + A - C - D
 * where C and D are the other two points of the rectange. The points
 * A and B are considered to be INSIDE the rectangle.
 */
template <typename T>
T
integral_image_area (typename Image<T>::ConstPtr sat,
    int x1, int y1, int x2, int y2, int cc = 0);

/**
 * Calculates and returns the dark channel of an RGB(A) image.
 * The dark channel is constructed by iterating over all pixels
 * and selecting is the smallest color component within a kernel
 * of fixed size for that pixel in the output image. Kernel size
 * is given in 'ks' as half size, i.e. full kernel has size 2*ks+1.
 *
 * The dark channel is discussed in a paper called:
 * "Single Image Haze Removal Using Dark Channel Prior"
 * The filter is slow but can be optimized using a technique called:
 * "A fast algorithm for local minimum and maximum filters
 *     on rectangular and octagonal kernels"
 */
template <typename T>
typename Image<T>::Ptr
dark_channel (typename Image<T>::ConstPtr image, int ks);

/**
 * Non-local means (NL-means) denoising operator described in:
 *
 *     A Non-Local Algorithm for Image Denoising
 *     A. Buades, B. Coll, J.-M. Morel
 *     In Proc. CVPR 2005
 *     http://www.ipol.im/pub/algo/bcm_non_local_means_denoising/
 *
 * Parameter 'sigma' acts as degree of filtering. 'cmp_win' is the size
 * of the window from which similarity is computed from. 'search_win'
 * is the size of the search window, which can be set to 0 to search in
 * the whole image.
 */
template <typename T>
typename Image<T>::Ptr
nl_means_filter (typename Image<T>::ConstPtr image,
    float sigma, int cmp_win = 1, int search_win = 8);
//nl_means_filter (typename Image<T>::ConstPtr image,
//    float sigma, int cmp_win = 3, int search_win = 10);

MVE_IMAGE_NAMESPACE_END
MVE_NAMESPACE_END

/* ------------------------- Implementation ----------------------- */

#include <cmath>
#include <stdexcept>

MVE_NAMESPACE_BEGIN
MVE_IMAGE_NAMESPACE_BEGIN

template <typename SRC, typename DST>
typename Image<DST>::Ptr
type_to_type_image (typename Image<SRC>::ConstPtr image)
{
    typename Image<DST>::Ptr out = Image<DST>::create();
    out->allocate(image->width(), image->height(), image->channels());
    int size = image->get_value_amount();
    SRC const* src_buf = image->get_data_pointer();
    DST* dst_buf = out->get_data_pointer();
    std::copy(src_buf, src_buf + size, dst_buf);
    return out;
}

/* ---------------------------------------------------------------- */

template <typename T>
void
find_min_max_value (typename mve::Image<T>::Ptr image, T* vmin, T* vmax)
{
    *vmin = std::numeric_limits<T>::max();
    *vmax = std::numeric_limits<T>::is_signed
        ? -std::numeric_limits<T>::max()
        : std::numeric_limits<T>::min();

    for (T const* ptr = image->begin(); ptr != image->end(); ++ptr)
    {
        if (*ptr < *vmin)
            *vmin = *ptr;
        if (*ptr > *vmax)
            *vmax = *ptr;
    }
}

/* ---------------------------------------------------------------- */

template <typename T>
typename Image<T>::Ptr
rescale (typename Image<T>::ConstPtr img, RescaleInterpolation interp,
    int width, int height)
{
    if (width == 0 && height == 0)
        throw std::invalid_argument("Invalid size request");

    /* Keep aspect ratio if one of width and height is 0. */
    if (width == 0)
        width = height * img->width() / img->height();
    else if (height == 0)
        height = width * img->height() / img->width();

    /* Scale down to an appropriate mipmap level for resizing. */
    if (interp == RESCALE_NEAREST || interp == RESCALE_LINEAR)
    {
        while (2 * width <= img->width() && 2 * height <= img->height())
            img = rescale_half_size<T>(img);
    }

    //std::cout << "Image mipmap size " << img->width() << "x"
    //    << img->height() << std::endl;

    typename Image<T>::Ptr out(Image<T>::create());
    out->allocate(width, height, img->channels());

    switch (interp)
    {
        case RESCALE_NEAREST:
            rescale_nearest<T>(img, out);
            break;
        case RESCALE_LINEAR:
            rescale_linear<T>(img, out);
            break;
#if 0
        case RESCALE_CUBIC:
            rescale_cubic(img, out);
            break;
#endif
        case RESCALE_GAUSSIAN:
            rescale_gaussian<T>(img, out);
            break;

        default:
            throw std::invalid_argument("Invalid interpolation type");
    }

    return out;
}

/* ---------------------------------------------------------------- */

template <typename T>
typename Image<T>::Ptr
rescale_half_size (typename Image<T>::ConstPtr img)
{
    int iw = img->width();
    int ih = img->height();
    int ic = img->channels();
    int ow = (iw + 1) >> 1;
    int oh = (ih + 1) >> 1;

    if (iw < 2 || ih < 2)
        throw std::invalid_argument("Invalid input image");

    typename Image<T>::Ptr out(Image<T>::create());
    out->allocate(ow, oh, ic);

    int outpos = 0;
    int rowstride = iw * ic;
    for (int y = 0; y < oh; ++y)
    {
        int irow1 = y * 2 * rowstride;
        int irow2 = irow1 + rowstride * (y * 2 + 1 < ih);

        for (int x = 0; x < ow; ++x)
        {
            int ipix1 = irow1 + x * 2 * ic;
            int ipix2 = irow2 + x * 2 * ic;
            int hasnext = (x * 2 + 1 < iw);

            for (int c = 0; c < ic; ++c)
                out->at(outpos++) = math::algo::interpolate<T>(
                    img->at(ipix1 + c), img->at(ipix1 + ic * hasnext + c),
                    img->at(ipix2 + c), img->at(ipix2 + ic * hasnext + c),
                    0.25f, 0.25f, 0.25f, 0.25f);
        }
    }

    return out;
}

/* ---------------------------------------------------------------- */

template <typename T>
typename Image<T>::Ptr
rescale_half_size_gaussian (typename Image<T>::ConstPtr img, float sigma)
{
    int iw = img->width();
    int ih = img->height();
    int ic = img->channels();
    int ow = (iw + 1) >> 1;
    int oh = (ih + 1) >> 1;

    if (iw < 2 || ih < 2)
        throw std::invalid_argument("Invalid input image");

    typename Image<T>::Ptr out(Image<T>::create());
    out->allocate(ow, oh, ic);

    /*
     * Weights w1 (4 center px), w2 (8 skewed px) and w3 (4 corner px).
     * Weights can be normalized by dividing with (4*w1 + 8*w2 + 4*w3).
     * Since the accumulator is used, normalization is not explicit.
     */
    float const w1 = std::exp(-0.5f / (2.0f * MATH_POW2(sigma)));
    float const w2 = std::exp(-2.5f / (2.0f * MATH_POW2(sigma)));
    float const w3 = std::exp(-4.5f / (2.0f * MATH_POW2(sigma)));

    int outpos = 0;
    int const rowstride = iw * ic;
    for (int y = 0; y < oh; ++y)
    {
        /* Init the four row pointers. */
        int y2 = (int)y << 1;
        T const* row[4];
        row[0] = &img->at(std::max(0, y2 - 1) * rowstride);
        row[1] = &img->at(y2 * rowstride);
        row[2] = &img->at(std::min((int)ih - 1, y2 + 1) * rowstride);
        row[3] = &img->at(std::min((int)ih - 1, y2 + 2) * rowstride);

        for (int x = 0; x < ow; ++x)
        {
            /* Init four pixel positions for each row. */
            int x2 = (int)x << 1;
            int xi[4];
            xi[0] = std::max(0, x2 - 1) * ic;
            xi[1] = x2 * ic;
            xi[2] = std::min((int)iw - 1, x2 + 1) * ic;
            xi[3] = std::min((int)iw - 1, x2 + 2) * ic;

            /* Accumulate 16 values in each channel. */
            for (int c = 0; c < ic; ++c)
            {
                math::Accum<T> accum(T(0));
                accum.add(row[0][xi[0] + c], w3);
                accum.add(row[0][xi[1] + c], w2);
                accum.add(row[0][xi[2] + c], w2);
                accum.add(row[0][xi[3] + c], w3);

                accum.add(row[1][xi[0] + c], w2);
                accum.add(row[1][xi[1] + c], w1);
                accum.add(row[1][xi[2] + c], w1);
                accum.add(row[1][xi[3] + c], w2);

                accum.add(row[2][xi[0] + c], w2);
                accum.add(row[2][xi[1] + c], w1);
                accum.add(row[2][xi[2] + c], w1);
                accum.add(row[2][xi[3] + c], w2);

                accum.add(row[3][xi[0] + c], w3);
                accum.add(row[3][xi[1] + c], w2);
                accum.add(row[3][xi[2] + c], w2);
                accum.add(row[3][xi[3] + c], w3);

                out->at(outpos++) = accum.normalized();
            }
        }
    }

    return out;
}

/* ---------------------------------------------------------------- */

template <typename T>
typename Image<T>::Ptr
rescale_half_size_subsample (typename Image<T>::ConstPtr img)
{
    int iw = img->width();
    int ih = img->height();
    int ic = img->channels();
    int ow = (iw + 1) >> 1;
    int oh = (ih + 1) >> 1;
    int irs = iw * ic; // input image row stride

    typename Image<T>::Ptr out(Image<T>::create());
    out->allocate(ow, oh, ic);

    int iter = 0; // Output image iterator
    for (int iy = 0; iy < ih; iy += 2)
    {
        int rowoff = iy * irs;
        int pixoff = rowoff;
        for (int ix = 0; ix < iw; ix += 2)
        {
            T const* iptr = &img->at(pixoff);
            T* optr = &out->at(iter);
            std::copy(iptr, iptr + ic, optr);
            pixoff += ic << 1;
            iter += ic;
        }
    }

    return out;
}

/* ---------------------------------------------------------------- */

template <typename T>
typename Image<T>::Ptr
rescale_double_size (typename Image<T>::ConstPtr img)
{
    int iw = img->width();
    int ih = img->height();
    int ic = img->channels();
    int ow = iw << 1;
    int oh = ih << 1;
    int irs = iw * ic;  // input image row stride

    typename Image<T>::Ptr out(Image<T>::create());
    out->allocate(ow, oh, ic);

    float w[4] = { 0.75f*0.75f, 0.25f*0.75f, 0.75f*0.25f, 0.25f*0.25f };

    T const* row1 = img->begin();
    T const* row2 = img->begin();

    int i = 0;
    for (int y = 0; y < oh; ++y)
    {
        /* Uneven row -> advance, even row -> swap. */
        if (y % 2)
            row2 = row1 + (y < oh - 1 ? irs : 0);
        else
            std::swap(row1, row2);

        T const* px[4] = { row1, row1, row2, row2 };
        for (int x = 0; x < ow; ++x)
        {
            /* Uneven pixel -> advance, even pixel -> swap. */
            if (x % 2)
            {
                int off = (x < ow - 1 ? ic : 0);
                px[1] = px[0] + off;
                px[3] = px[2] + off;
            }
            else
            {
                std::swap(px[0], px[1]);
                std::swap(px[2], px[3]);
            }

            for (int c = 0; c < ic; ++c, ++i)
                out->at(i) = math::algo::interpolate
                    (px[0][c], px[1][c], px[2][c], px[3][c],
                    w[0], w[1], w[2], w[3]);
        }
    }

    return out;
}

/* ---------------------------------------------------------------- */

template <typename T>
typename Image<T>::Ptr
rescale_double_size_supersample (typename Image<T>::ConstPtr img)
{
    int iw = img->width();
    int ih = img->height();
    int ic = img->channels();
    int ow = iw << 1;
    int oh = ih << 1;

    typename Image<T>::Ptr out(Image<T>::create());
    out->allocate(ow, oh, ic);

    int witer = 0;
    for (int y = 0; y < oh; ++y)
    {
        bool nexty = (y + 1 < oh);
        int yoff[2] = { iw * (y >> 1), iw * ((y + nexty) >> 1) };
        for (int x = 0; x < ow; ++x)
        {
            bool nextx = (x + 1 < ow);
            int xoff[2] = { x >> 1, (x + nextx) >> 1 };
            T const* val[4] =
            {
                &img->at(yoff[0] + xoff[0], 0),
                &img->at(yoff[0] + xoff[1], 0),
                &img->at(yoff[1] + xoff[0], 0),
                &img->at(yoff[1] + xoff[1], 0)
            };

            for (int c = 0; c < ic; ++c, ++witer)
                out->at(witer) = math::algo::interpolate
                    (val[0][c], val[1][c], val[2][c], val[3][c],
                    0.25f, 0.25f, 0.25f, 0.25f);
        }
    }

    return out;
}

/* ---------------------------------------------------------------- */

template <typename T>
void
rescale_nearest (typename Image<T>::ConstPtr img, typename Image<T>::Ptr out)
{
    if (img->channels() != out->channels())
        throw std::invalid_argument("Image channel mismatch");

    int iw = img->width();
    int ih = img->height();
    int ic = img->channels();
    int ow = out->width();
    int oh = out->height();

    int outpos = 0;
    for (int y = 0; y < oh; ++y)
    {
        float ly = ((float)y + 0.5f) * (float)ih / (float)oh;
        int iy = static_cast<int>(ly);
        for (int x = 0; x < ow; ++x)
        {
            float lx = ((float)x + 0.5f) * (float)iw / (float)ow;
            int ix = static_cast<int>(lx);
            for (int c = 0; c < ic; ++c)
                out->at(outpos++) = img->at(ix,iy,c);
        }
    }
}

/* ---------------------------------------------------------------- */

template <typename T>
void
rescale_linear (typename Image<T>::ConstPtr img, typename Image<T>::Ptr out)
{
    if (img->channels() != out->channels())
        throw std::invalid_argument("Image channel mismatch");

    int iw = img->width();
    int ih = img->height();
    int ic = img->channels();
    int ow = out->width();
    int oh = out->height();
    T* out_ptr = out->get_data_pointer();

    int outpos = 0;
    for (int y = 0; y < oh; ++y)
    {
        float fy = ((float)y + 0.5f) * (float)ih / (float)oh;
        for (int x = 0; x < ow; ++x, outpos += ic)
        {
            float fx = ((float)x + 0.5f) * (float)iw / (float)ow;
            img->linear_at(fx - 0.5f, fy - 0.5f, out_ptr + outpos);
        }
    }
}

/* ---------------------------------------------------------------- */

template <typename T>
T
gaussian_kernel (typename Image<T>::ConstPtr img,
    float x, float y, int c, float sigma)
{
    int w = img->width();
    int h = img->height();

    /* Calculate kernel size for geometric gaussian (see bilateral.h). */
    float ks = sigma * 2.884f;

    /* Calculate min/max kernel position. */
    float kx_min = std::floor(x - ks);
    float kx_max = std::ceil(x + ks - 1.0f);
    float ky_min = std::floor(y - ks);
    float ky_max = std::ceil(y + ks - 1.0f);

    int kxi_min = static_cast<int>(std::max(0.0f, kx_min));
    int kxi_max = static_cast<int>(std::min((float)w - 1.0f, kx_max));
    int kyi_min = static_cast<int>(std::max(0.0f, ky_min));
    int kyi_max = static_cast<int>(std::min((float)h - 1.0f, ky_max));

    /* Determine pixel weight for kernel bounaries. */
    float wx_start = kx_min > 0.0f ? kx_min + 1.0f + ks - x : 1.0f;
    float wx_end = kx_max < (float)w - 1.0f ? ks + x - kx_max : 1.0f;
    float wy_start = ky_min > 0.0f ? ky_min + 1.0f + ks - y : 1.0f;
    float wy_end = ky_max < (float)h - 1.0f ? ks + y - ky_max : 1.0f;

    /*
    std::cout << "Gaussian for pixel (" << x << "," << y << "): "
        << "Min/max: (" << kxi_min << "," << kyi_min
        << ") / (" << kxi_max << "," << kyi_max << "), weights: "
        << wx_start << " " << wx_end << " " << wy_start << " " << wy_end
        << ", kernel " << ks
        << std::endl;
    */

    /* Apply kernel. */
    math::Accum<T> accum(0);
    for (int yi = kyi_min; yi <= kyi_max; ++yi)
        for (int xi = kxi_min; xi <= kxi_max; ++xi)
        {
            float weight = 1.0f;
            weight *= (xi == kxi_min ? wx_start : 1.0f);
            weight *= (xi == kxi_max ? wx_end : 1.0f);
            weight *= (yi == kyi_min ? wy_start : 1.0f);
            weight *= (yi == kyi_max ? wy_end : 1.0f);
            float dx = static_cast<float>(xi) + 0.5f - x;
            float dy = static_cast<float>(yi) + 0.5f - y;
            weight *= math::algo::gaussian_xx(dx * dx + dy * dy, sigma);

            accum.add(img->at(xi, yi, c), weight);
        }

    return accum.normalized();
}

/* ---------------------------------------------------------------- */

template <typename T>
void
rescale_gaussian (typename Image<T>::ConstPtr img,
    typename Image<T>::Ptr out, float sigma_factor)
{
    if (img->channels() != out->channels())
        throw std::invalid_argument("Image channels mismatch");

    int ow = out->width();
    int oh = out->height();
    int oc = out->channels();

    /* Choose gaussian sigma parameter according to scale factor. */
    float scale_x = (float)img->width() / (float)ow;
    float scale_y = (float)img->height() / (float)oh;
    float sigma = sigma_factor * std::max(scale_x, scale_y) / 2.0f;
    //std::cout << "Effective sigma: " << sigma << std::endl;

    /* Iterate pixels of dest image and convolute with gaussians on input. */
    int i = 0;
    for (int y = 0; y < oh; ++y)
        for (int x = 0; x < ow; ++x, ++i)
        {
            float xf = ((float)x + 0.5f) * scale_x;
            float yf = ((float)y + 0.5f) * scale_y;
            for (int c = 0; c < oc; ++c)
                out->at(i, c) = gaussian_kernel<T>(img, xf, yf, c, sigma);
        }
}

/* ---------------------------------------------------------------- */

template <typename T>
typename Image<T>::Ptr
crop (typename Image<T>::ConstPtr img,
    int left, int top, int width, int height)
{
    typename Image<T>::Ptr out(Image<T>::create());
    out->allocate(width, height, img->channels());

    int iw = img->width();
    int ih = img->height();
    int ic = img->channels();

    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
        {
            int ix = left + x;
            int iy = top + y;
            for (int c = 0; c < ic; ++c)
            {
                T value(0);
                if (ix < iw && iy < ih)
                    value = img->at(ix,iy,c);
                out->at(x,y,c) = value;
            }
        }

    return out;
}

/* ---------------------------------------------------------------- */

template <typename T>
typename Image<T>::Ptr
blur_gaussian (typename Image<T>::ConstPtr in, float sigma)
{
    if (!in.get())
        throw std::invalid_argument("NULL image given");

    /* Small sigmas result in literally no change. */
    if (MATH_EPSILON_EQ(sigma, 0.0f, 0.1f))
        return in->duplicate();

    int w = in->width();
    int h = in->height();
    int c = in->channels();
    int ks = std::ceil(sigma * 2.884f); // Cap kernel at 1/128
    std::vector<float> kernel(ks + 1);

    /* Fill kernel values. */
    for (int i = 0; i < ks + 1; ++i)
        kernel[i] = math::algo::gaussian((float)i, sigma);

#if 1 // Separated kernel implementation

    /* Convolve the image in x direction. */
    typename Image<T>::Ptr sep(Image<T>::create(w, h, c));
    int px = 0;
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x, ++px)
            for (int cc = 0; cc < (int)c; ++cc)
            {
                math::Accum<T> accum(T(0));
                for (int i = -ks; i <= ks; ++i)
                {
                    int idx = math::algo::clamp(x + i, 0, (int)w - 1);
                    accum.add(in->at(y * w + idx, cc), kernel[std::abs(i)]);
                }
                sep->at(px, cc) = accum.normalized();
            }

    /* Convolve the image in y direction. */
    typename Image<T>::Ptr out(Image<T>::create(w, h, c));
    px = 0;
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x, ++px)
            for (int cc = 0; cc < c; ++cc)
            {
                math::Accum<T> accum(T(0));
                for (int i = -ks; i <= ks; ++i)
                {
                    int idx = math::algo::clamp(y + i, 0, (int)h - 1);
                    accum.add(sep->at(idx * w + x, cc), kernel[std::abs(i)]);
                }
                out->at(px, cc) = accum.normalized();
            }

#else // Non-separated kernel implementation

    typename Image<T>::Ptr out(Image<T>::create(w, h, c));
    int px = 0;
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x, ++px)
            for (int cc = 0; cc < c; ++cc)
            {
                math::Accum<T> accum(T(0));

                for (int ky = -ks; ky <= ks; ++ky)
                    for (int kx = -ks; kx <= ks; ++kx)
                    {
                        int idx_x = math::algo::clamp(x + kx, 0, (int)w - 1);
                        int idx_y = math::algo::clamp(y + ky, 0, (int)h - 1);
                        accum.add(in->at(idx_y * w + idx_x, cc),
                            kernel[std::abs(kx)] * kernel[std::abs(ky)]);
                    }

                out->at(px, cc) = accum.normalized();
            }

#endif

    return out;
}

/* ---------------------------------------------------------------- */

template <typename T>
typename Image<T>::Ptr
blur_boxfilter (typename Image<T>::ConstPtr in, int ks)
{
    if (!in.get())
        throw std::invalid_argument("NULL image given");

    int w(in->width());
    int h(in->height());
    int c(in->channels());
    int wc = w * c;

#if 1
    /* Super-fast separated kernel implementation. */
    typename Image<T>::Ptr sep(Image<T>::create(w, h, c));
    math::Accum<T>* accums = new math::Accum<T>[c];
    T const* row = &in->at(0);
    T* outrow = &sep->at(0);
    for (int y = 0; y < h; ++y, row += w * c, outrow += w * c)
    {
        for (int cc = 0; cc < c; ++cc) // Reset accumulators
            accums[cc] = math::Accum<T>(T(0));

        for (int i = 0; i < ks; ++i)
            for (int cc = 0; cc < c; ++cc)
                accums[cc].add(row[i * c + cc], 1.0f);

        for (int x = 0; x < w; ++x)
        {
            if (x + ks < w - 1)
                for (int cc = 0; cc < c; ++cc)
                    accums[cc].add(row[(x + ks) * c + cc], 1.0f);
            if (x > ks)
                for (int cc = 0; cc < c; ++cc)
                    accums[cc].sub(row[(x - ks - 1) * c + cc], 1.0f);
            for (int cc = 0; cc < c; ++cc)
                outrow[x * c + cc] = accums[cc].normalized();
        }
    }

    /* Second filtering pass with kernel in y-direction. */
    typename Image<T>::Ptr out(Image<T>::create(w, h, c));
    T const* col = &sep->at(0);
    T* outcol = &out->at(0);
    for (int x = 0; x < w; ++x, col += c, outcol += c)
    {
        for (int cc = 0; cc < c; ++cc)
            accums[cc] = math::Accum<T>(T(0));

        for (int i = 0; i < ks; ++i)
            for (int cc = 0; cc < c; ++cc)
                accums[cc].add(col[i * wc + cc], 1.0f);

        for (int y = 0; y < h; ++y)
        {
            if (y + ks < h - 1)
                for (int cc = 0; cc < c; ++cc)
                    accums[cc].add(col[(y + ks) * wc + cc], 1.0f);
            if (y > ks)
                for (int cc = 0; cc < c; ++cc)
                    accums[cc].sub(col[(y - ks - 1) * wc + cc], 1.0f);
            for (int cc = 0; cc < c; ++cc)
                outcol[y * wc + cc] = accums[cc].normalized();
        }
    }

    delete [] accums;
#endif


#if 0 // Slow and naive, but simple implementation
    typename Image<T>::Ptr out(Image<T>::create(w, h, c));
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            for (int cc = 0; cc < c; ++cc)
            {
                math::Accum<T> accum(T(0));
                for (int ky = -ks; ky <= ks; ++ky)
                    for (int kx = -ks; kx <= ks; ++kx)
                    {
                        int idx_x = math::algo::clamp(x + kx, 0, (int)w - 1);
                        int idx_y = math::algo::clamp(y + ky, 0, (int)h - 1);
                        accum.add(in->at(idx_x, idx_y, cc), 1.0f);
                    }
                out->at(x, y, cc) = accum.normalized();
            }
#endif

    return out;
}

/* ---------------------------------------------------------------- */

template <typename T>
typename Image<T>::Ptr
rotate (typename Image<T>::ConstPtr image, RotateType type)
{
    if (!image.get())
        throw std::invalid_argument("NULL image given");

    int iw = image->width();
    int ih = image->height();
    int ic = image->channels();
    int ow = type == ROTATE_180 ? iw : ih;
    int oh = type == ROTATE_180 ? ih : iw;

    typename Image<T>::Ptr ret(Image<T>::create());
    ret->allocate(ow, oh, ic);

    int idx = 0;
    for (int y = 0; y < ih; ++y)
        for (int x = 0; x < iw; ++x, idx += ic)
        {
            int dx = x;
            int dy = y;
            switch (type)
            {
                case ROTATE_180:  dx = iw - x - 1; dy = ih - y - 1; break;
                case ROTATE_CW:   dx = ih - y - 1; dy = x;          break;
                case ROTATE_CCW:  dx = y;          dy = iw - x - 1; break;
                case ROTATE_SWAP: dx = y;          dy = x;          break;
                default: break;
            }

            T const* in_pixel = &image->at(idx);
            T* out_pixel = &ret->at(dx, dy, 0);
            std::copy(in_pixel, in_pixel + ic, out_pixel);
        }

    return ret;
}

/* ---------------------------------------------------------------- */

template <typename T>
void
flip (typename Image<T>::Ptr image, FlipType type)
{
    if (type == FLIP_NONE)
        return;
    if (!image.get())
        throw std::invalid_argument("NULL image given");

    bool const fh = type & FLIP_HORIZONTAL;
    bool const fv = type & FLIP_VERTICAL;
    int const iw = image->width();
    int const ih = image->height();
    int const ic = image->channels();
    int const max_x = fh ? iw / 2 : iw;
    int const max_y = fv && !fh ? ih / 2 : ih;

    for (int y = 0; y < max_y; ++y)
        for (int x = 0; x < max_x; ++x)
        {
            int dx = (type & FLIP_HORIZONTAL ? iw - x - 1 : x);
            int dy = (type & FLIP_VERTICAL ? ih - y - 1 : y);
            T* src = &image->at(x, y, 0);
            T* dst = &image->at(dx, dy, 0);
            std::swap_ranges(src, src + ic, dst);
        }
}

/* ---------------------------------------------------------------- */

template <typename T>
inline T
desaturate_maximum (T const* v)
{
    return *std::max_element(v, v + 3);
}

template <typename T>
inline T
desaturate_lightness (T const* v)
{
    T const* max = std::max_element(v, v + 3);
    T const* min = std::min_element(v, v + 3);
    return math::algo::interpolate(*max, *min, 0.5f, 0.5f);
}

template <typename T>
inline T
desaturate_luminosity (T const* v)
{
    return math::algo::interpolate(v[0], v[1], v[2], 0.21f, 0.72f, 0.07f);
}

template <typename T>
inline T
desaturate_luminance (T const* v)
{
    return math::algo::interpolate(v[0], v[1], v[2], 0.30f, 0.59f, 0.11f);
}

template <typename T>
inline T
desaturate_average (T const* v)
{
    float third(1.0f / 3.0f);
    return math::algo::interpolate(v[0], v[1], v[2], third, third, third);
}

/* ---------------------------------------------------------------- */

template <typename T>
typename Image<T>::Ptr
desaturate (typename Image<T>::ConstPtr img, DesaturateType type)
{
    if (!img.get())
        throw std::invalid_argument("NULL image given");

    int ic = img->channels();
    if (ic != 3 && ic != 4)
        throw std::invalid_argument("Image must be RGB or RGBA");

    bool has_alpha = (ic == 4);

    typename Image<T>::Ptr out(Image<T>::create());
    out->allocate(img->width(), img->height(), 1 + has_alpha);

    typedef T(*DesaturateFunc)(T const*);
    DesaturateFunc func;
    switch (type)
    {
        case DESATURATE_MAXIMUM: func = desaturate_maximum<T>; break;
        case DESATURATE_LIGHTNESS: func = desaturate_lightness<T>; break;
        case DESATURATE_LUMINOSITY: func = desaturate_luminosity<T>; break;
        case DESATURATE_LUMINANCE: func = desaturate_luminance<T>; break;
        case DESATURATE_AVERAGE: func = desaturate_average<T>; break;
        default: throw std::invalid_argument("Invalid desaturate type");
    }

    int outpos = 0;
    int inpos = 0;
    int pixels = img->get_pixel_amount();
    for (int i = 0; i < pixels; ++i)
    {
        T const* v = &img->at(inpos);
        out->at(outpos) = func(v);

        if (has_alpha)
            out->at(outpos + 1) = img->at(inpos + 3);

        outpos += 1 + has_alpha;
        inpos += 3 + has_alpha;
    }

    return out;
}

/* ---------------------------------------------------------------- */

template <typename T>
typename Image<T>::Ptr
expand_grayscale (typename Image<T>::ConstPtr image)
{
    if (!image.get())
        throw std::invalid_argument("NULL image given");

    int ic = image->channels();
    if (ic != 1 && ic != 2)
        throw std::invalid_argument("Image must be in G or GA");

    bool has_alpha = (ic == 2);

    typename Image<T>::Ptr out(Image<T>::create());
    out->allocate(image->width(), image->height(), 3 + has_alpha);

    int pixels = image->get_pixel_amount();
    for (int i = 0; i < pixels; ++i)
    {
        out->at(i, 0) = image->at(i, 0);
        out->at(i, 1) = image->at(i, 0);
        out->at(i, 2) = image->at(i, 0);
        if (has_alpha)
            out->at(i, 3) = image->at(i, 1);
    }

    return out;
}

/* ---------------------------------------------------------------- */

template <typename T>
void
reduce_alpha (typename mve::Image<T>::Ptr img)
{
    int channels = img->channels();
    if (channels != 2 && channels != 4)
        throw std::invalid_argument("Image must be in GA or RGBA");
    img->delete_channel(channels - 1);
}

/* ---------------------------------------------------------------- */

template <typename T>
typename mve::Image<T>::Ptr
sobel_edge (typename mve::Image<T>::ConstPtr img)
{
    int w = img->width();
    int h = img->height();
    int c = img->channels(); // pixel stride
    int rs = w * c; // row stride

    double const max_value = static_cast<double>(std::numeric_limits<T>::max());
    typename mve::Image<T>::Ptr out = mve::Image<T>::create(w, h, c);
    T* out_ptr = out->get_data_pointer();

    int pos = 0;
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x, pos += c)
        {
            if (y == 0 || y == h-1 || x == 0 || x == w-1)
            {
                std::fill(out_ptr + pos, out_ptr + pos + c, T(0));
                continue;
            }

            for (int cc = 0; cc < c; ++cc)
            {
                int i = pos + cc;
                double gx = 1.0 * (double)img->at(i+c-rs)
                    - 1.0 * (double)img->at(i-c-rs)
                    + 2.0 * (double)img->at(i+c)
                    - 2.0 * (double)img->at(i-c)
                    + 1.0 * (double)img->at(i+c+rs)
                    - 1.0 * (double)img->at(i-c+rs);
                double gy = 1.0 * (double)img->at(i+rs-c)
                    - 1.0 * (double)img->at(i-rs-c)
                    + 2.0 * (double)img->at(i+rs)
                    - 2.0 * (double)img->at(i-rs)
                    + 1.0 * (double)img->at(i+rs+c)
                    - 1.0 * (double)img->at(i-rs+c);
                double g = std::sqrt(gx * gx + gy * gy);
                out_ptr[i] = static_cast<T>(std::min(max_value, g));
            }
        }

    return out;
}

/* ---------------------------------------------------------------- */

template <typename T>
typename Image<T>::Ptr
subtract (typename Image<T>::Ptr i1, typename Image<T>::Ptr i2)
{
    int iw = i1->width();
    int ih = i1->height();
    int ic = i1->channels();

    if (!i1.get() || !i2.get())
        throw std::invalid_argument("NULL image given");

    if (iw != i2->width() || ih != i2->height() || ic != i2->channels())
        throw std::invalid_argument("Image dimensions do not match");

    typename Image<T>::Ptr out(Image<T>::create());
    out->allocate(iw, ih, ic);

    // FIXME: That needs speedup! STL algo with pointers.
    for (int i = 0; i < i1->get_value_amount(); ++i)
        out->at(i) = i1->at(i) - i2->at(i);

    return out;
}

/* ---------------------------------------------------------------- */

template <typename T>
typename Image<T>::Ptr
difference (typename Image<T>::Ptr i1, typename Image<T>::Ptr i2)
{
    int iw = i1->width();
    int ih = i1->height();
    int ic = i1->channels();

    if (!i1.get() || !i2.get())
        throw std::invalid_argument("NULL image given");

    if (iw != i2->width() || ih != i2->height() || ic != i2->channels())
        throw std::invalid_argument("Image dimensions do not match");

    typename Image<T>::Ptr out(Image<T>::create());
    out->allocate(iw, ih, ic);

    for (int i = 0; i < i1->get_value_amount(); ++i)
    {
        if (i1->at(i) < i2->at(i))
            out->at(i) = i2->at(i) - i1->at(i);
        else
            out->at(i) = i1->at(i) - i2->at(i);
    }

    return out;
}

/* ---------------------------------------------------------------- */

template <typename T>
void
gamma_correct (typename Image<T>::Ptr image, T const& power)
{
    math::algo::foreach_constant_power<T> f(power);
    std::for_each(image->begin(), image->end(), f);
}

/* ---------------------------------------------------------------- */

template <typename IN, typename OUT>
typename Image<OUT>::Ptr
integral_image (typename Image<IN>::ConstPtr image)
{
    if (!image.get())
        throw std::invalid_argument("NULL image given");

    int w = image->width();
    int h = image->height();
    int c = image->channels();
    int wc = w * c; // row stride

    typename Image<OUT>::Ptr ret(Image<OUT>::create());
    ret->allocate(w, h, c);

    /* Input image row and destination image rows. */
    std::vector<OUT> zeros(w * c, OUT(0));
    IN const* inrow = image->get_data_pointer();
    OUT* dest = ret->get_data_pointer();
    OUT* prev = &zeros[0];

    /*
     * I(x,y) = i(x,y) + I(x-1,y) + I(x,y-1) - I(x-1,y-1)
     */
    for (int y = 0; y < h; ++y)
    {
        /* Calculate first pixel in row. */
        for (int cc = 0; cc < c; ++cc)
            dest[cc] = static_cast<OUT>(inrow[cc]) + prev[cc];
        //std::transform(prev, prev + c, inrow, dest, std::plus<T>());
        /* Calculate all following pixels in row. */
        for (int i = c; i < wc; ++i)
            dest[i] = inrow[i] + prev[i] + dest[i - c] - prev[i - c];

        prev = dest;
        dest += wc;
        inrow += wc;
    }

    return ret;
}

/* ---------------------------------------------------------------- */

template <typename T>
T
integral_image_area (typename Image<T>::ConstPtr sat,
    int x1, int y1, int x2, int y2, int cc)
{
    int w = sat->width();
    int c = sat->channels();
    int wc = w * c; // row stride

    T ret = sat->at(y2 * wc + x2 * c + cc); // bottom-right
    if (x1 > 0)
        ret -= sat->at(y2 * wc + (x1-1) * c + cc); // bottom-left
    if (y1 > 0)
        ret -= sat->at((y1-1) * wc + x2 * c + cc); // top-right
    if (x1 > 0 && y1 > 0)
        ret += sat->at((y1-1) * wc + (x1-1) * c + cc); // top-left
    return ret;
}

/* ---------------------------------------------------------------- */

template <typename T>
typename Image<T>::Ptr
dark_channel (typename Image<T>::ConstPtr image, int ks)
{
    if (!image.get())
        throw std::invalid_argument("NULL image given");
    if (ks < 0 || ks > 256)
        throw std::invalid_argument("Invalid kernel size given");

    int w = image->width();
    int h = image->height();

    typename Image<T>::Ptr ret(Image<T>::create(w, h, 1));

    int idx = 0;
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x, ++idx)
        {
            int x1, x2, y1, y2;
            math::algo::kernel_region(x, y, ks, w, h, x1, x2, y1, y2);
            T min(std::numeric_limits<T>::max());
            for (int cy = y1; cy <= y2; ++cy)
                for (int cx = x1; cx <= x2; ++cx)
                    for (int c = 0; c < 3; ++c)
                        min = std::min(min, image->at(cx, cy, c));
            ret->at(idx) = min;
        }

    return ret;
}

/* ---------------------------------------------------------------- */

template <typename T>
float
nl_means_intern_distance (Image<T> const& img,
    int x1, int y1, int x2, int y2, int win)
{
    int w = img.width();
    int c = img.channels();
    int wc = w * c;
    int wlen = 2 * win + 1;
    T const* p1 = img.get_data_pointer() + (y1 - win) * wc + (x1 - win) * c;
    T const* p2 = img.get_data_pointer() + (y2 - win) * wc + (x2 - win) * c;

    float ret = 0.0f;
    for (int y = 0; y < wlen; ++y, p1 += wc, p2 += wc)
        for (int x = 0; x < wlen * c; ++x)
            ret += math::algo::fastpow((float)*(p1 + x) - (float)*(p2 + x), 2);
    return ret;
}

template <typename T>
typename Image<T>::Ptr
nl_means_filter (typename Image<T>::ConstPtr image,
    float sigma, int cmp_win, int search_win)
{
    if (!image.get())
        throw std::invalid_argument("NULL image given");
    if (cmp_win <= 0 || search_win < 0)
        throw std::invalid_argument("Invalid window sizes");

    int const w = (int)image->width();
    int const h = (int)image->height();
    int const c = (int)image->channels();
    typename Image<T>::Ptr ret = Image<T>::create(w, h, c);

    int const cws = cmp_win * 2 + 1; // compare window size
    int const cwl = cws * cws; // length of compare window
    float const cwlc = (float)(cwl * c); // size of compare window

    float const sigma2 = sigma * sigma;
    float const filter = 0.55f * sigma;
    float const filter2 = filter * filter * cwlc;

//#pragma omp parallel for
    for (int y = 0; y < h; ++y)
    {
        int idx = y * w * c;
        for (int x = 0; x < w; ++x)
        {
            /* Reduce size of comparison window near boundary. */
            int cwin = math::algo::min(cmp_win, x, y);
            cwin = math::algo::min(cwin, w-1-x, h-1-y);

            /* Define research zone (search window). */
            int const swx1 = std::max(x - search_win, cwin);
            int const swx2 = std::min(x + search_win, w-1-cwin);
            int const swy1 = std::max(y - search_win, cwin);
            int const swy2 = std::min(y + search_win, h-1-cwin);

            /* Provide accumulators for pixel values at (x,y). */
            std::vector<math::Accum<T> > avec(c, math::Accum<T>(T(0)));

            /* Walk over search window. */
            float max_weight = 0.0f;
            for (int swy = swy1; swy <= swy2; ++swy)
                for (int swx = swx1; swx <= swx2; ++swx)
                {
                    if (swx == x && swy == y)
                        continue;

                    float dist2 = nl_means_intern_distance
                        (*image, x, y, swx, swy, cwin);
                    dist2 = std::max(0.0f, dist2 - 2.0f * cwlc * sigma2);
                    float weight = std::exp(-dist2 / filter2);
                    max_weight = std::max(max_weight, weight);

                    /* Index of center pixel. */
                    T const* tmp_ptr = &image->at(swx, swy, 0);
                    for (int cc = 0; cc < c; ++cc, ++tmp_ptr)
                        avec[cc].add(*tmp_ptr, weight);
                }

            for (int cc = 0; cc < c; ++cc, ++idx)
            {
                if (MATH_FLOAT_EQ(max_weight, 0.0f))
                    avec[cc].add(image->at(idx), 1.0f);
                else
                    avec[cc].add(image->at(idx), max_weight);
                ret->at(idx) = avec[cc].normalized();
            }
        }
    }

    return ret;
}

/* ---------------------------------------------------------------- */

/* FIXME: Does not support irregular principal point. */

template <typename T>
typename Image<T>::Ptr
image_undistort (typename Image<T>::ConstPtr img, CameraInfo const& cam)
{
    int w = img->width();
    int h = img->height();
    int c = img->channels();
    int D = std::max(w,h);

    /* major hacking -> focal_length^2 */
    float k0 = cam.dist[0] * MATH_POW2(cam.flen);
    float k1 = cam.dist[1] * MATH_POW2(cam.flen);

    typename Image<T>::Ptr out = Image<T>::create(w, h, c);
    T* out_ptr = out->get_data_pointer();
    out->fill(T(0));

    int outpos = 0;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++, outpos += c)
        {
            float p3d[3] =
            {
                (float)x - 0.5f * (float)w,
                (float)y - 0.5f * (float)h,
                cam.flen * D
            };

            float s1 = p3d[2]*p3d[2] + k1*(p3d[0] * p3d[0] + p3d[1] * p3d[1]);
            float s2 = p3d[2]*p3d[2] + k0*(p3d[0] * p3d[0] + p3d[1] * p3d[1]);

            p3d[2] *= s2;
            p3d[0] *= s1 * cam.flen * D/p3d[2];
            p3d[1] *= s1 * cam.flen * D/p3d[2];

            p3d[0] += 0.5f * (float)w;
            p3d[1] += 0.5f * (float)h;

            float xc = p3d[0];
            float yc = p3d[1];

            if (xc < 0.0f || xc > w-1 || yc < 0.0f || yc > h-1)
                continue;
            img->linear_at(xc, yc, out_ptr + outpos);
        }

    return out;
}

/* ---------------------------------------------------------------- */

/* FIXME: Does not support irregular principal point. */

template <typename T>
typename Image<T>::Ptr
image_undistort_noah (typename Image<T>::ConstPtr img, CameraInfo const& cam)
{
    if (!img.get())
        throw std::invalid_argument("NULL image given");

    float k0 = cam.dist[0];
    float k1 = cam.dist[1];

    if (k0 == 0.0f && k1 == 0.0f)
        return img->duplicate();

    int w = img->width();
    int h = img->height();
    int c = img->channels();
    float fw = static_cast<float>(w);
    float fh = static_cast<float>(h);
    float fw2 = fw * 0.5f;
    float fh2 = fh * 0.5f;
    float noah_flen = cam.flen * std::max(fw, fh);

    float f2inv = 1.0f / (noah_flen * noah_flen);

    typename Image<T>::Ptr out = Image<T>::create(w, h, c);
    T* out_ptr = out->get_data_pointer();
    out->fill(T(0));

    int outpos = 0;
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x, outpos += c)
        {
            float xc = (float)x - fw2;
            float yc = (float)y - fh2;
            float r2 = (xc * xc + yc * yc) * f2inv;
            float factor = 1.0f + k0 * r2 + k1 * r2 * r2;
            xc = xc * factor + fw2;
            yc = yc * factor + fh2;

            if (xc < 0.0f || xc > fw - 1.0f || yc < 0.0f || yc > fh - 1.0f)
                continue;
            img->linear_at(xc, yc, out_ptr + outpos);
        }

    return out;
}

MVE_IMAGE_NAMESPACE_END
MVE_NAMESPACE_END

#endif /* MVE_IMAGE_TOOLS_HEADER */
