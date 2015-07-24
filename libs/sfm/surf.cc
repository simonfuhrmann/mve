/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <iostream>

#include "util/timer.h"
#include "math/functions.h"
#include "math/vector.h"
#include "math/matrix.h"
#include "math/matrix_tools.h"
#include "mve/image.h"
#include "mve/image_tools.h"
#include "mve/image_drawing.h"
#include "sfm/defines.h"
#include "sfm/surf.h"

SFM_NAMESPACE_BEGIN

namespace
{
    /* Kernel sizes per octave (given in 1/3 of the full size). */
    int const kernel_sizes[4][4] =
    {
        {  3,  5,  7,  9 },  //  9  15  21  27
        {  5,  9, 13, 17 },  // 15  27  39  51
        {  9, 17, 25, 33 },  // 27  51  75  99
        { 17, 33, 49, 65 }   // 51  99 147 195
    };
}  // namespace

/* ---------------------------------------------------------------- */

Surf::Surf (Options const& options)
    : options(options)
{
    if (this->options.debug_output)
        this->options.verbose_output = true;
}

/* ---------------------------------------------------------------- */

void
Surf::process (void)
{
    this->keypoints.clear();
    this->descriptors.clear();
    this->octaves.clear();

    util::WallTimer timer, total_timer;

    /* Compute Hessian response maps and find SS maxima (SURF 3.3). */
    if (this->options.verbose_output)
    {
        std::cout << "SURF: Creating 4 octaves (0 to 4)..." << std::endl;
    }
    timer.reset();
    this->create_octaves();
    if (this->options.debug_output)
    {
        std::cout << "SURF: Creating octaves took "
            << timer.get_elapsed() << " ms." << std::endl;
    }

    /* Detect local extrema in the SS of Hessian response maps. */
    if (this->options.debug_output)
    {
        std::cout << "SURF: Detecting local extrema..." << std::endl;
    }
    timer.reset();
    this->extrema_detection();
    if (this->options.debug_output)
    {
        std::cout << "SURF: Extrema detection took "
            << timer.get_elapsed() << " ms." << std::endl;
    }

    /* Sub-pixel keypoint localization and filtering of weak keypoints. */
    if (this->options.debug_output)
    {
        std::cout << "SURF: Localizing and filtering keypoints..." << std::endl;
    }
    timer.reset();
    this->keypoint_localization_and_filtering();
    this->octaves.clear();
    if (this->options.debug_output)
    {
        std::cout << "SURF: Localization and filtering took "
            << timer.get_elapsed() << " ms." << std::endl;
    }

    /* Compute the SURF descriptor for the keypoint location. */
    if (this->options.verbose_output)
    {
        std::cout << "SURF: Generating keypoint descriptors..." << std::endl;
    }
    timer.reset();
    this->descriptor_assignment();
    if (this->options.debug_output)
    {
        std::cout << "SURF: Generated " << this->descriptors.size()
            << " descriptors, took " << timer.get_elapsed() << "ms."
            << std::endl;
    }
    if (this->options.verbose_output)
    {
        std::cout << "SURF: Generated " << this->descriptors.size()
            << " descriptors from " << this->keypoints.size() << " keypoints,"
            << " took " << total_timer.get_elapsed() << "ms." << std::endl;
    }

    /* Cleanup. */
    this->sat.reset();
}

/* ---------------------------------------------------------------- */

void
Surf::set_image (mve::ByteImage::ConstPtr image)
{
    /* Desaturate input image. */
    if (image->channels() != 1 && image->channels() != 3)
        throw std::invalid_argument("Gray or color image expected");
    if (image->channels() == 3)
        image = mve::image::desaturate<uint8_t>(image,
            mve::image::DESATURATE_LIGHTNESS);

    /* Build summed area table (SAT). */
    //util::WallTimer timer;
    this->sat = mve::image::integral_image<uint8_t,SatType>(image);
    //std::cout << "Creating SAT image took "
    //    << timer.get_elapsed() << " ms." << std::endl;
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
    for (int octave = 0; octave < 4; ++octave)
        for (int sample = 0; sample < 4; ++sample)
            this->create_response_map(octave, sample);
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
    typedef Octave::RespType RespType;

    /* Filter size. The actual kernel side length is 3 * fs. */
    int const fs = kernel_sizes[o][k];
    /* The sample spacing for the octaves. */
    int const step = math::fastpow(2, o);
    /* Weight to balance between real gaussian kernel and approximated one. */
    RespType const weight = 0.912;  // See SURF 3.3 (4).
    /* NormalizationKernel width is 3 * fs, height is 2 * fs - 1. */
    RespType const inv_karea = 1.0 / (fs * (2 * fs - 1));

    /* Original dimensions and octave dimensions. */
    int const w = this->sat->width();
    int const h = this->sat->height();
    int ow = w;
    int oh = h;
    for (int i = 0; i < o; ++i)
    {
        ow = (ow + 1) >> 1;
        oh = (oh + 1) >> 1;
    }

    /* Generate the response maps. */
    Octave::RespImage::Ptr img = Octave::RespImage::create(ow, oh, 1);
    int const border = fs + fs / 2 + 1;
    for (int y = 0, i = 0; y < h; y += step)
        for (int x = 0; x < w; x += step, ++i)
        {
            if (x < border || x + border >= w || y < border || y + border >= h)
            {
                img->at(i) = 0.0f;
                continue;
            }

            SatType dxx = this->filter_dxx(fs, x, y);
            SatType dyy = this->filter_dyy(fs, x, y);
            SatType dxy = this->filter_dxy(fs, x, y);
            RespType dxx_t = static_cast<RespType>(dxx) * inv_karea;
            RespType dyy_t = static_cast<RespType>(dyy) * inv_karea;
            RespType dxy_t = static_cast<RespType>(dxy) * inv_karea;
            /* Compute the determinant of the hessian. */
            img->at(i) = dxx_t * dyy_t - weight * dxy_t * dxy_t;
            /* The laplacian can be computed as dxx_t + dyy_t. */
            // float laplacian = dxx_t + dyy_t;
        }

    this->octaves[o].imgs[k] = img;
}

/* ---------------------------------------------------------------- */

Surf::SatType
Surf::filter_dxx (int fs, int x, int y)
{
    int const w = this->sat->width();
    int const fs2 = fs / 2;
    int const row1 = (x - fs - fs2 - 1) + w * (y - fs);
    int const row2 = row1 + w * (fs + fs - 1);

    Surf::SatType const v0 = this->sat->at(row1);
    Surf::SatType const v1 = this->sat->at(row1 + fs);
    Surf::SatType const v2 = this->sat->at(row1 + 2 * fs);
    Surf::SatType const v3 = this->sat->at(row1 + 3 * fs);
    Surf::SatType const v4 = this->sat->at(row2);
    Surf::SatType const v5 = this->sat->at(row2 + fs);
    Surf::SatType const v6 = this->sat->at(row2 + 2 * fs);
    Surf::SatType const v7 = this->sat->at(row2 + 3 * fs);

    Surf::SatType ret = 0;
    ret += v5 + v0 - v4 - v1;
    ret -= 2 * (v6 + v1 - v5 - v2);
    ret += v7 + v2 - v6 - v3;
    return ret;
}

Surf::SatType
Surf::filter_dyy (int fs, int x, int y)
{
    int const w = this->sat->width();
    int const fs2 = fs / 2;
    int const row1 = (x - fs) + w * (y - fs - fs2 - 1);
    int const row2 = row1 + w * fs;
    int const row3 = row2 + w * fs;
    int const row4 = row3 + w * fs;

    Surf::SatType const v0 = this->sat->at(row1);
    Surf::SatType const v1 = this->sat->at(row2);
    Surf::SatType const v2 = this->sat->at(row3);
    Surf::SatType const v3 = this->sat->at(row4);
    Surf::SatType const v4 = this->sat->at(row1 + fs + fs - 1);
    Surf::SatType const v5 = this->sat->at(row2 + fs + fs - 1);
    Surf::SatType const v6 = this->sat->at(row3 + fs + fs - 1);
    Surf::SatType const v7 = this->sat->at(row4 + fs + fs - 1);

    Surf::SatType ret = 0;
    ret += v5 + v0 - v1 - v4;
    ret -= 2 * (v6 + v1 - v2 - v5);
    ret += v7 + v2 - v3 - v6;
    return ret;
}

Surf::SatType
Surf::filter_dxy (int fs, int x, int y)
{
    int const w = this->sat->width();
    int const row1 = (x - fs - 1) + w * (y - fs - 1);
    int const row2 = row1 + w * fs;
    int const row3 = row2 + w;
    int const row4 = row3 + w * fs;

    Surf::SatType ret = 0;
    Surf::SatType v0 = this->sat->at(row1);
    Surf::SatType v1 = this->sat->at(row1 + fs);
    Surf::SatType v2 = this->sat->at(row2);
    Surf::SatType v3 = this->sat->at(row2 + fs);
    ret += v3 + v0 - v2 - v1;

    v0 = this->sat->at(row1 + fs + 1);
    v1 = this->sat->at(row1 + fs + fs + 1);
    v2 = this->sat->at(row2 + fs + 1);
    v3 = this->sat->at(row2 + fs + fs + 1);
    ret -= v3 + v0 - v2 - v1;

    v0 = this->sat->at(row3);
    v1 = this->sat->at(row3 + fs);
    v2 = this->sat->at(row4);
    v3 = this->sat->at(row4 + fs);
    ret -= v3 + v0 - v2 - v1;

    v0 = this->sat->at(row3 + fs + 1);
    v1 = this->sat->at(row3 + fs + fs + 1);
    v2 = this->sat->at(row4 + fs + 1);
    v3 = this->sat->at(row4 + fs + fs + 1);
    ret += v3 + v0 - v2 - v1;

    return ret;
}

/* ---------------------------------------------------------------- */

void
Surf::extrema_detection (void)
{
    /*
     * At this stage each octave contains 4 scale space samples and local
     * maxima/minima in the approximated DoG function need to be found.
     * To this end, a simple non-maximum suppression technique is applied.
     */
    for (std::size_t o = 0; o < this->octaves.size(); ++o)
    {
        int const width = this->octaves[o].imgs[0]->width();
        int const height = this->octaves[o].imgs[0]->height();
        for (std::size_t s = 1; s < 3; ++s)
        {
            Octave::RespImage::ConstPtr ss = this->octaves[o].imgs[s];
            Octave::RespType const* ptr = &ss->at(0);
            for (int y = 0; y < height; ++y, ptr += width)
                for (int x = 1; x + 1 < width; ++x)
                    if (ptr[x] > ptr[x-1] && ptr[x] > ptr[x + 1])
                        this->check_maximum(o, s, x, y);
        }
    }
}

/* ---------------------------------------------------------------- */

void
Surf::check_maximum(int o, int s, int x, int y)
{
    /*
     * Assumes that given coordinates are within bounds and a 1 pixel
     * boundary for comarisons in x, y and s direction.
     */
    int const w = this->octaves[o].imgs[s]->width();
    int off1[8] = { -w - 1, -w, -w + 1, -1,    +1, w - 1, w, w + 1 };
    int off2[9] = { -w - 1, -w, -w + 1, -1, 0, +1, w - 1, w, w + 1 };

    Octave::RespType const* ptr;
    int const off = x + y * w;

    /* Perform NMS check on the candidate sample first. */
    ptr = this->octaves[o].imgs[s]->get_data_pointer() + off;
    Octave::RespType value = ptr[0];
    for (int i = 0; i < 8; ++i)
        if (ptr[off1[i]] >= value)
            return;
    /* Perform NMS check on the above sample. */
    ptr = this->octaves[o].imgs[s - 1]->get_data_pointer() + off;
    for (int i = 0; i < 9; ++i)
        if (ptr[off2[i]] >= value)
            return;
    /* Perform NMS check on the below sample. */
    ptr = this->octaves[o].imgs[s + 1]->get_data_pointer() + off;
    for (int i = 0; i < 9; ++i)
        if (ptr[off2[i]] >= value)
            return;

    /* Seems like we found a keypoint. */
    Keypoint kp;
    kp.octave = o;
    kp.sample = static_cast<float>(s);
    kp.x = static_cast<float>(x);
    kp.y = static_cast<float>(y);
    this->keypoints.push_back(kp);
}

/* ---------------------------------------------------------------- */

void
Surf::keypoint_localization_and_filtering (void)
{
    Keypoints filtered_keypoints;
    filtered_keypoints.reserve(this->keypoints.size());
    for (std::size_t i = 0; i < this->keypoints.size(); ++i)
    {
        Keypoint kp = this->keypoints[i];
        bool solve_good = this->keypoint_localization(&kp);
        if (!solve_good)
            continue;
        filtered_keypoints.push_back(kp);
    }
    std::swap(this->keypoints, filtered_keypoints);
}

/* ---------------------------------------------------------------- */

bool
Surf::keypoint_localization (Keypoint* kp)
{
    int const sample = static_cast<int>(kp->sample);
    int const w = this->octaves[kp->octave].imgs[sample]->width();
    //int const h = this->octaves[kp->octave].imgs[sample]->height();
    int off = static_cast<int>(kp->x) + static_cast<int>(kp->y) * w;
    Octave::RespType const *s0, *s1, *s2;
    s0 = this->octaves[kp->octave].imgs[sample - 1]->get_data_pointer() + off;
    s1 = this->octaves[kp->octave].imgs[sample]->get_data_pointer() + off;
    s2 = this->octaves[kp->octave].imgs[sample + 1]->get_data_pointer() + off;

    float N9[3][9] =
    {
        { s0[-1-w], s0[-w], s0[1-w], s0[-1], s0[0], s0[1], s0[w-1], s0[w], s0[1+w] },
        { s1[-1-w], s1[-w], s1[1-w], s1[-1], s1[0], s1[1], s1[w-1], s1[w], s1[1+w] },
        { s2[-1-w], s2[-w], s2[1-w], s2[-1], s2[0], s2[1], s2[w-1], s2[w], s2[1+w] }
    };

    /* Switch to processing in double. Determinant can be very large. */
    math::Vec3d vec_b;
    math::Matrix3d mat_a;
    vec_b[0] = -(N9[1][5] - N9[1][3]) * 0.5;  // 1st deriv x.
    vec_b[1] = -(N9[1][7] - N9[1][1]) * 0.5;  // 1st deriv y.
    vec_b[2] = -(N9[2][4] - N9[0][4]) * 0.5;  // 1st deriv s.

    mat_a[0] = N9[1][3] - 2.0 * N9[1][4] + N9[1][5];                 // xx
    mat_a[1] = (N9[1][8] - N9[1][6] - N9[1][2] + N9[1][0]) * 0.25;   // xy
    mat_a[2] = (N9[2][5] - N9[2][3] - N9[0][5] + N9[0][3]) * 0.25;   // xs
    mat_a[3] = mat_a[1];                                             // yx
    mat_a[4] = N9[1][1] - 2.0 * N9[1][4] + N9[1][7];                 // yy
    mat_a[5] = (N9[2][7] - N9[2][1] - N9[0][7] + N9[0][1]) * 0.25;   // ys
    mat_a[6] = mat_a[2];                                             // sx
    mat_a[7] = mat_a[5];                                             // sy
    mat_a[8] = N9[0][4] - 2.0 * N9[1][4] + N9[2][4];                 // ss

    /* Compute determinant to detect singular matrix. */
    double det_a = math::matrix_determinant(mat_a);
    //std::cout << "Determinant: " << det_a << std::endl;
    if (MATH_EPSILON_EQ(det_a, 0.0f, 1.0e-5f))
    {
        //std::cout << "Rejection keypoint: det = " << det_a << std::endl;
        return false;
    }

    /* Invert the matrix to get the accurate keypoint. */
    mat_a = math::matrix_inverse(mat_a, det_a);
    math::Vec3d vec_x = mat_a * vec_b;

    /* Reject keypoint if location is too far off original point. */
    if (vec_x.maximum() > 0.5 || vec_x.minimum() < -0.5f)
        return false;

    /* Compute actual DoG value at accurate keypoint x. */
    float dog_value = N9[1][4] - 0.5 * vec_b.dot(vec_x);
    if (dog_value < this->options.contrast_threshold)
    {
        //std::cout << "Rejection keypoint: Contrast Thres" << std::endl;
        return false;
    }

    /* Update keypoint. */
    float sampling = static_cast<float>(math::fastpow(2, kp->octave));
    kp->x = (kp->x + vec_x[0]) * sampling;
    kp->y = (kp->y + vec_x[1]) * sampling;
    // OpenSURF code: ipt.scale = static_cast<float>((0.1333f) * (m->filter + xi * filterStep));
    kp->sample += vec_x[2]; // Is this correct? No! FIXME!

    if (kp->x < 0.0f || kp->x + 1.0f > this->sat->width()
        || kp->y < 0.0f || kp->y + 1.0f > this->sat->height())
    {
        //std::cout << "SURF: Rejection keypoint: OOB "
        //    << kp->x << " " << kp->y  << " / " << vec_x[0] << " "
        //    << vec_x[1] << std::endl;
        return false;
    }

    return true;
}

/* ---------------------------------------------------------------- */

void
Surf::descriptor_assignment (void)
{
    this->descriptors.clear();
    this->descriptors.reserve(keypoints.size());
    for (std::size_t i = 0; i < this->keypoints.size(); ++i)
    {
        Keypoint const& kp = this->keypoints[i];

        /* Copy over the basic information to the descriptor. */
        Descriptor descr;
        descr.x = kp.x;
        descr.y = kp.y;

        /*
         * The scale is obtained from the filter size. The smallest filter in
         * SURF has size 9 and corresponds to a scale of 1.2. Thus, the
         * scale of a filter with size X has a scale of X * 1.2 / 9.
         */
        int sample = static_cast<int>(kp.sample + 0.5f);
        descr.scale = 3 * kernel_sizes[kp.octave][sample] * 1.2f / 9.0f;

        /* Find the orientation of the keypoint. */
        if (!this->descriptor_orientation(&descr))
            continue;

        /* Compute descriptor relative to orientation. */
        if (!this->descriptor_computation(&descr,
            this->options.use_upright_descriptor))
            continue;

        this->descriptors.push_back(descr);
    }
}

/* ---------------------------------------------------------------- */

bool
Surf::descriptor_orientation (Descriptor* descr)
{
    int const descr_x = static_cast<int>(descr->x + 0.5f);
    int const descr_y = static_cast<int>(descr->y + 0.5f);
    int const descr_scale = static_cast<int>(descr->scale);
    int const width = this->sat->width();
    int const height = this->sat->height();

    /*
     * Pre-computed gaussian weights for the (circular) window. The gaussian
     * is computed as exp((dx^2 + dy^2) / (2 * sigma^2) with sigma = 2.5.
     */
    float const gaussian[109] = { 0.0658748, 0.0982736, 0.12493, 0.135335,
        0.12493, 0.0982736, 0.0658748, 0.0773047, 0.135335, 0.201897, 0.256661,
        0.278037, 0.256661, 0.201897, 0.135335, 0.0773047, 0.0658748, 0.135335,
        0.236928, 0.353455, 0.449329, 0.486752, 0.449329, 0.353455, 0.236928,
        0.135335, 0.0658748, 0.0982736, 0.201897, 0.353455, 0.527292, 0.67032,
        0.726149, 0.67032, 0.527292, 0.353455, 0.201897, 0.0982736, 0.12493,
        0.256661, 0.449329, 0.67032, 0.852144, 0.923116, 0.852144, 0.67032,
        0.449329, 0.256661, 0.12493, 0.135335, 0.278037, 0.486752, 0.726149,
        0.923116, 1, 0.923116, 0.726149, 0.486752, 0.278037, 0.135335, 0.12493,
        0.256661, 0.449329, 0.67032, 0.852144, 0.923116, 0.852144, 0.67032,
        0.449329, 0.256661, 0.12493, 0.0982736, 0.201897, 0.353455, 0.527292,
        0.67032, 0.726149, 0.67032, 0.527292, 0.353455, 0.201897, 0.0982736,
        0.0658748, 0.135335, 0.236928, 0.353455, 0.449329, 0.486752, 0.449329,
        0.353455, 0.236928, 0.135335, 0.0658748, 0.0773047, 0.135335, 0.201897,
        0.256661, 0.278037, 0.256661, 0.201897, 0.135335, 0.0773047, 0.0658748,
        0.0982736, 0.12493, 0.135335, 0.12493, 0.0982736, 0.0658748 };

    /*
     * At least a 12 * scale pixel kernel for the support circle is needed.
     * Additionally, computing the Haar Wavelet response uses a kernel size
     * of 4 * scale pixel. That makes a total of (6 + 2) * scale pixel on
     * either side of the kernel spacing. One additional pixel is needed
     * to the upper-left side to simplify integral image access.
     */
    int const spacing = 8 * descr_scale + 1;
    if (descr_x < spacing || descr_y < spacing
        || descr_x + spacing >= width || descr_y + spacing >= height)
        return false;

    /* The number of samples is constant, depending on radius. */
#define NUM_SAMPLES 109
    float dx[NUM_SAMPLES], dy[NUM_SAMPLES];

    /*
     * Iterate over the pixels of a circle with radius 6 * scale
     * and compute Haar Wavelet responses in x- and y-direction.
     */
    for (int ry = -5, index = 0; ry <= 5; ++ry)
        for (int rx = -5; rx <= 5; ++rx)
        {
            if (rx * rx + ry * ry >= 36)
                continue;
            this->filter_dx_dy(descr_x + rx * descr_scale,
                descr_y + ry * descr_scale, 2 * descr_scale,
                dx + index, dy + index);
            dx[index] *= gaussian[index];
            dy[index] *= gaussian[index];
            index += 1;
        }

    /*
     * Iterate in a sliding window over the 109 extracted samples
     * in order to find a dominant orientation for the keypoint.
     */
    double const window_increment = MATH_PI / 8.0;
    double const window_halfsize = MATH_PI / 6.0;
    double best_dx = 0.0, best_dy = 0.0;
    double best_length = 0.0f;
    for (double deg = -MATH_PI; deg < MATH_PI; deg += window_increment)
    {
        /* Evaluate samples for this window. */
        double const win_start = deg - window_halfsize;
        double const win_end = deg + window_halfsize;
        double sum_dx = 0.0f, sum_dy = 0.0f;
        for (int i = 0; i < NUM_SAMPLES; ++i)
        {
            double const sample_deg = std::atan2(dy[i], dx[i]);
            double const sample_deg_p = sample_deg + 2.0 * MATH_PI;
            double const sample_deg_m = sample_deg - 2.0 * MATH_PI;
            if ((sample_deg > win_start && sample_deg < win_end)
                || (sample_deg_p > win_start && sample_deg_p < win_end)
                || (sample_deg_m > win_start && sample_deg_m < win_end))
            {
                sum_dx += dx[i];
                sum_dy += dy[i];
            }
        }

        /* Total vector length of dx/dy sums defines dominance. */
        double const length = sum_dx * sum_dx + sum_dy * sum_dy;
        if (length > best_length)
        {
            best_dx = sum_dx;
            best_dy = sum_dy;
            best_length = length;
        }
    }

    /* TODO: Threshold on the vector length? */
    descr->orientation = std::atan2(best_dy, best_dx);
    return true;
}

/* ---------------------------------------------------------------- */

void
Surf::filter_dx_dy (int x, int y, int fs, float* dx, float* dy)
{
    int const width = this->sat->width();
    SatType const* ptr = this->sat->begin();

    /*
     * To have a center pixel, filter size needs to be odd.
     * To ensure symmetry in the filters, we include the center row in
     * both sides of the dy filter, and the center column in both sides
     * of the dx filter, which cancels them out. However, this costs
     * four additional lookups (12 instead of 8).
     */
    SatType const* iter = ptr + (x - fs - 1) + (y - fs - 1) * width;
    SatType x1 = *iter; iter += fs;
    SatType x2 = *iter; iter += 1;
    SatType x3 = *iter; iter += fs;
    SatType x4 = *iter; iter += (width - 1) * (2 * fs + 1);

    SatType x5 = *iter; iter += fs;
    SatType x6 = *iter; iter += 1;
    SatType x7 = *iter; iter += fs;
    SatType x8 = *iter;

    iter = ptr + (x - fs - 1) + (y - 1) * width;
    SatType y1 = *iter; iter += 2 * fs + 1;
    SatType y2 = *iter; iter += width - 2 * fs - 1;
    SatType y3 = *iter; iter += 2 * fs + 1;
    SatType y4 = *iter;

    /*
     * Normalize filter by size "(2 * fs + 1) * fs" and normalize discrete
     * derivative with distance between the Wavelet box centers "fs + 1".
     */
    float norm = static_cast<float>((2 * fs + 1) * fs * (fs + 1));
    *dx = static_cast<float>((x8 + x2 - x4 - x6) - (x7 + x1 - x3 - x5)) / norm;
    *dy = static_cast<float>((x8 + y1 - x5 - y2) - (y4 + x1 - y3 - x4)) / norm;
}

/* ---------------------------------------------------------------- */

bool
Surf::descriptor_computation (Descriptor* descr, bool upright)
{
    int const descr_scale = static_cast<int>(descr->scale);
    int const width = this->sat->width();
    int const height = this->sat->height();

    /*
     * Size of the descriptor is 20s (10s to each side). The Wavelet filter
     * have size 2s (1s to each side), plus one additional pixel for simpler
     * integral image lookup. Since the window can be rotated, 4s additional
     * pixel are needed.
     */
    float const spacing = static_cast<float>(15 * descr_scale + 1);
    if (descr->x < spacing || descr->y < spacing
        || descr->x + spacing >= width || descr->y + spacing > height)
        return false;

    float sin_ori = 0.0f, cos_ori = 1.0f;
    if (!upright)
    {
        sin_ori = std::sin(descr->orientation);
        cos_ori = std::cos(descr->orientation);
    }

    /* Interest point region has size 20 * scale. */
    descr->data.fill(0.0f);
    float* descr_iter = &descr->data[0];
    for (int y = -10; y < 10; ++y)
    {
        for (int x = -10; x < 10; ++x)
        {
            /* Rotate sample coordinate. */
            int rot_x = static_cast<int>(math::round(descr->x
                + (cos_ori * (x + 0.5f) - sin_ori * (y + 0.5f))
                * descr_scale));
            int rot_y = static_cast<int>(math::round(descr->y
                + (sin_ori * (x + 0.5f) + cos_ori * (y + 0.5f))
                * descr_scale));

            /* Obtain and rotate gradient. */
            float dx, dy;
            this->filter_dx_dy(rot_x, rot_y, descr_scale, &dx, &dy);
            float ori_dx = cos_ori * dx + sin_ori * dy;
            float ori_dy = -sin_ori * dx + cos_ori * dy;

            /* Keypoints are weighted with a Gaussian, sigma = 3.3*s. */
            float weight = std::exp(-static_cast<float>(x * x + y * y)
                / math::fastpow(2.0f * 3.3f, 2));
            *(descr_iter++) += weight * ori_dx;
            *(descr_iter++) += weight * ori_dy;
            *(descr_iter++) += weight * std::abs(ori_dx);
            *(descr_iter++) += weight * std::abs(ori_dy);

            if ((x + 10) % 5 != 4)
                descr_iter -= 4;
        }
        if ((y + 10) % 5 != 4)
            descr_iter -= 4 * 4;
    }

    /* Normalize descriptor, reject too small descriptors. */
    float norm = descr->data.square_norm();
    if (MATH_EPSILON_EQ(norm, 0.0f, 1e-8))  // TODO: Play with this.
        return false;
    descr->data /= std::sqrt(norm);

    return true;
}

SFM_NAMESPACE_END
