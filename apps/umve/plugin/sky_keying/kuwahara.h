// Might eventually be added to image_tools.h, once everything's ready

#include <iostream>
#include <limits>
#include <complex>
#include "util/exception.h"
#include "math/accum.h"
#include "math/functions.h"
#include "mve/defines.h"
#include "mve/image.h"

/*
 * Performs a symmetric reflexion for filter kernel coordinates.
 * In case the specified pixel is located within the image, nothing happens.
 *
 * Note that the image's width and height are necessary in case a reflection
 * at the right/bottom border occurs.
 *
 * @param x The filter's X-coordinate (potentially not part of the image)
 * @param y The filter's Y-coordinate (potentially not part of the image)
 * @param w The image's width
 * @param h The image's height
 *
 */
void
reflect(int const &x,
        int const &y,
        std::size_t const &w,
        std::size_t const &h,
        std::size_t &dest_x,
        std::size_t &dest_y)
{
    dest_x = x;
    dest_y = y;
    /* Symmetric reflection (left/top border) */
    if (x < 0)
        dest_x = -x - 1;
    if (y < 0)
        dest_y = -y - 1;
    /* Symmetric reflection (right/bottom border) */
    if (x >= (int) w)
        dest_x -= 2 * (x - w) + 1;
    if (y >= (int) h)
        dest_y -= 2 * (y - h) + 1;
    return;
}

/*
 * Analyzes a Kuwahara kernel quadrant and determines various characteristics
 * such as mean values or the brightness variance.
 *
 * This acts as a helper function for smooth_kuwahara.
 *
 * @param startX Top-left x-coordinate of the quadrant
 * @param startY Top-left y-coordinate of the quadrant
 * @param endX Bottom-right x-coordinate of the quadrant
 * @param endY Bottom-right y-coordinate of the quadrant
 * @param imageWidth Image width
 * @param imageHeight Image height
 * @param in Image to be filtered
 * @param windowSize Kernel window size
 * @param vVarDest Destination to write the brightness ('v' as in HSV) variance
 * @param rMeanDest Destination to write the mean value for 'red'
 * @param gMeanDest Destination to write the mean value for 'green'
 * @param bMeanDest Destination to write the mean value for 'blue'
 */
template <typename T>
void
analyze_quadrant(int const &startX,
        int const &startY,
        int const &endX,
        int const &endY,
        std::size_t const &imageWidth,
        std::size_t const &imageHeight,
        typename mve::Image<T>::ConstPtr const in,
        std::size_t const windowWidth,
        float &vVarDest,
        float &rMeanDest,
        float &gMeanDest,
        float &bMeanDest)
{
    int rSum = 0, gSum = 0, bSum = 0, vSum = 0;
    float vVar = 0;

    std::size_t n = (windowWidth + 1) * (windowWidth + 1);

    for (int yKernel = startY; yKernel <= endY; ++yKernel)
        for (int xKernel = startX; xKernel <= endX; ++xKernel)
        {
            std::size_t dx = 0, dy = 0;
            reflect(xKernel, yKernel, imageWidth, imageHeight, dx, dy);
            rSum += in->at(dx, dy, 0);
            gSum += in->at(dx, dy, 1);
            bSum += in->at(dx, dy, 2);
            vSum += std::max(std::max(in->at(dx, dy, 0), in->at(dx, dy, 1)), in->at(dx, dy, 2));
        }

    float rMean = rSum / (float) n;
    float gMean = gSum / (float) n;
    float bMean = bSum / (float) n;
    float vMean = vSum / (float) n;


    #pragma omp parallel for
    for (int yKernel = startY; yKernel <= endY; ++yKernel)
        for (int xKernel = startX; xKernel <= endX; ++xKernel)
        {
            std::size_t dx = 0, dy = 0;
            reflect(xKernel, yKernel, imageWidth, imageHeight, dx, dy);
            T v = std::max(std::max(in->at(dx, dy, 0), in->at(dx, dy, 1)), in->at(dx, dy, 2));
            vVar += ((v - vMean) * (v - vMean)) / (n - 1);
        }

    if (vVar < vVarDest)
    {
        vVarDest = vVar;
        rMeanDest = rMean;
        gMeanDest = gMean;
        bMeanDest = bMean;
    }

    return;
}



/*
 * Performs a non-linear Kuwahara smoothing filter on an RGB image.
 *
 * @param in Image to be filtered
 * @param kerDim One dimension of the square kernel window (width or height)
 */
template <typename T>
typename mve::Image<T>::Ptr
smooth_kuwahara(typename mve::Image<T>::ConstPtr in, std::size_t kerDim)
{
    // No image selected
    if (in == NULL)
        return mve::Image<T>::create();

    std::size_t windowWidth = kerDim / 2;
    // Kernel dimension has no effect
    if (!windowWidth)
        return in->duplicate();

    std::size_t imageWidth = in->width();
    std::size_t imageHeight = in->height();

    typename mve::Image<T>::Ptr img = mve::Image<T>::create(imageWidth, imageHeight, 3);

    #pragma omp parallel for schedule(dynamic, 1) collapse(2)
    for (std::size_t y = 0; y < imageHeight; ++y)
        for (std::size_t x = 0; x < imageWidth; ++x)
        {
                float vVar = std::numeric_limits<float>::max();
                float rMean = std::numeric_limits<float>::max();
                float gMean = std::numeric_limits<float>::max();
                float bMean = std::numeric_limits<float>::max();

                // Analyze all four Kuwahara regions
                analyze_quadrant<T>(x, y, x + windowWidth, y + windowWidth, imageWidth, imageHeight, in, windowWidth, vVar, rMean, gMean, bMean);
                analyze_quadrant<T>(x - windowWidth, y, x, y + windowWidth, imageWidth, imageHeight, in, windowWidth, vVar, rMean, gMean, bMean);
                analyze_quadrant<T>(x - windowWidth, y - windowWidth, x, y, imageWidth, imageHeight, in, windowWidth, vVar, rMean, gMean, bMean);
                analyze_quadrant<T>(x, y - windowWidth, x + windowWidth, y, imageWidth, imageHeight, in, windowWidth, vVar, rMean, gMean, bMean);

                img->at(x, y, 0) = rMean;
                img->at(x, y, 1) = gMean;
                img->at(x, y, 2) = bMean;
        }
    return img;
}
