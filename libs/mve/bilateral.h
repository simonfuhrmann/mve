/*
 * Implementation of a bilateral filter for images and depth maps.
 *
 * Bilateral filtering smoothes similar regions (similar in color value)
 * but preserves edges (depth/color discontinuities). This is achieved
 * by combining geometric closeness (gaussian smoothing) with photometric
 * closeness (edge preservation).
 *
 * Written by Simon Fuhrmann.
 */

#ifndef MVE_BILATERAL2_HEADER
#define MVE_BILATERAL2_HEADER

#include "math/vector.h"
#include "math/accum.h"
#include "mve/defines.h"
#include "mve/image.h"

MVE_NAMESPACE_BEGIN
MVE_IMAGE_NAMESPACE_BEGIN

/**
 * Bilateral filter for images of type T, where the first N channels
 * are processed and affected by the filter.
 *
 * Geometric closeness is the distance in image space and controlled
 * by 'gc_sigma', larger sigma results in larger kernel and heavier
 * blur. Photometric closeness is evaluated using euclidean distance
 * norm in image color space. Useful values for 'gc_sigma' are in
 * [0, 20], typical values for 'pc_sigma' are in [0.05, 0.5].
 *
 * One important thing is that multiple channels are used at once
 * when evaluating photometric closeness to avoid color bleeding.
 * Usual images are in RGB color space; however other color spaces
 * where the euclidean metric correlates with human perception is more
 * suitable and generates more pleasant results. An new photometric
 * closeness functor is neccessary then.
 */
template <typename T, int N>
typename Image<T>::Ptr
bilateral_filter (typename Image<T>::ConstPtr img,
    float gc_sigma, float pc_sigma);

/* ------------------------- Details ------------------------------ */

/**
 * Generic bilateral filter kernel for center pixel (cx,cy) with
 * kernel size 'ks', geometric closeness function 'gcf' and
 * photometric closeness function 'pcf'.
 */
template <typename T, int N, typename GCF, typename PCF>
math::Vector<T,N>
bilateral_kernel (Image<T> const& img, int cx, int cy, int ks,
    GCF& gcf, PCF& pcf);

/**
 * A default geometric closeness functor for bilateral filtering.
 * The functor returns a weight, calculated with a gaussian function
 * using the given sigma. The weight is exponentially decreasing
 * depending on the distance between (cx,cy) and (x,y).
 */
struct BilateralGeomCloseness
{
    float sigma;

    BilateralGeomCloseness (float gc_sigma);
    float operator() (std::size_t cx, std::size_t cy,
        std::size_t x, std::size_t y);
};

/**
 * A default photometric closeness functor for bilateral filtering.
 * The functor returns a weight, calculated with a gaussion function
 * using the given sigma. The weight is exponentially decreasing
 * depending on the euclidean distance of the photometric (color)
 * values in 'cv' and 'v'.
 */
template <typename T, int N>
struct BilateralPhotoCloseness
{
    float sigma;

    BilateralPhotoCloseness (float pc_sigma);
    float operator() (math::Vector<T,N> const& cv, math::Vector<T,N> const& v);
};

MVE_IMAGE_NAMESPACE_END
MVE_NAMESPACE_END

/* -------------------------- Implementation ---------------------- */

MVE_NAMESPACE_BEGIN
MVE_IMAGE_NAMESPACE_BEGIN

inline
BilateralGeomCloseness::BilateralGeomCloseness (float gc_sigma)
    : sigma(gc_sigma)
{
}

inline float
BilateralGeomCloseness::operator() (std::size_t cx, std::size_t cy,
    std::size_t x, std::size_t y)
{
    float dx = (float)cx - (float)x;
    float dy = (float)cy - (float)y;
    return math::algo::gaussian_xx(dx * dx + dy * dy, this->sigma);
}

template <typename T, int N>
inline
BilateralPhotoCloseness<T,N>::BilateralPhotoCloseness (float pc_sigma)
    : sigma(pc_sigma)
{
}

template <typename T, int N>
inline float
BilateralPhotoCloseness<T,N>::operator() (math::Vector<T,N> const& cv,
    math::Vector<T,N> const& v)
{
    math::Vector<float,N> cvf(cv);
    math::Vector<float,N> vf(v);
    return math::algo::gaussian_xx((cvf - vf).square_norm(), this->sigma);
}

/* ---------------------------------------------------------------- */

template <typename T, int N, typename GCF, typename PCF>
math::Vector<T,N>
bilateral_kernel (Image<T> const& img, int cx, int cy, int ks,
    GCF& gcf, PCF& pcf)
{
    int const w = img.width();
    int const h = img.height();
    int x1, x2, y1, y2;
    math::algo::kernel_region(cx, cy, ks, w, h, &x1, &x2, &y1, &y2);

    math::Vector<T, N> center_value;
    int idx = (cy * w + cx) * img.channels();
    for (int c = 0; c < N; ++c)
        center_value[c] = img.at(idx + c);

    math::Vector<math::Accum<T>, N> accums(math::Accum<T>(T(0)));
    for (int y = y1; y <= y2; ++y)
        for (int x = x1; x <= x2; ++x)
        {
            math::Vector<T, N> local_value;
            std::size_t lidx = (y * w + x) * img.channels();
            for (int c = 0; c < N; ++c)
                local_value[c] = img.at(lidx + c);

            float gc = gcf(cx, cy, x, y);
            float pc = pcf(center_value, local_value);
            float pixel_weight = gc * pc;
            for (int c = 0; c < N; ++c)
                accums[c].add(local_value[c], pixel_weight);
        }

    /* Prepare result vector. */
    math::Vector<T,N> ret;
    for (int i = 0; i < N; ++i)
        ret[i] = accums[i].normalized();
    return ret;
}

/* ---------------------------------------------------------------- */

template <typename T, int N>
typename Image<T>::Ptr
bilateral_filter (typename Image<T>::ConstPtr img,
    float gc_sigma, float pc_sigma)
{
    /* Copy original image. */
    typename Image<T>::Ptr ret(Image<T>::create(*img));

    /*
     * Calculate kernel size for geometric gaussian.
     * Kernel is cut off at y=1/N, x = sigma * sqrt(2 * ln N).
     * For N=256: x = sigma * 3.33.
     * For N=128: x = sigma * 3.12.
     * For N=64: x = sigma * 2.884.
     * For N=32: x = sigma * 2.63.
     * For N=16: x = sigma * 2.355.
     * For N=8: x = sigma * 2.04.
     * For N=4: x = sigma * 1.67.
     */
    float ks_f = gc_sigma * 2.884f;
    std::size_t ks = std::ceil(ks_f);

    //std::cout << "Bilateral filtering image with GC "
    //    << gc_sigma << " and PC " << pc_sigma
    //    << ", kernel " << (ks*2+1) << "^2 pixels." << std::endl;

    /* Use standard closeness functors. */
    typedef BilateralGeomCloseness GCF;
    typedef BilateralPhotoCloseness<T,N> PCF;
    GCF gcf(gc_sigma);
    PCF pcf(pc_sigma);

    /* Apply kernel to each pixel. */
    for (int y = 0; y < img->height(); ++y)
        for (int x = 0; x < img->width(); ++x)
        {
            math::Vector<T,N> v = bilateral_kernel<T, N, GCF, PCF>
                (*img, x, y, ks, gcf, pcf);
            for (int c = 0; c < N; ++c)
                ret->at(x, y, c) = v[c];
        }

    return ret;
}

MVE_IMAGE_NAMESPACE_END
MVE_NAMESPACE_END

#endif /* MVE_BILATERAL2_HEADER */
