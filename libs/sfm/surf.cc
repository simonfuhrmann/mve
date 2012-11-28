#include <iostream> // RM

#include "util/timer.h" // RM
#include "math/vector.h"
#include "math/matrix.h"
#include "math/matrixtools.h"
#include "mve/image.h"
#include "mve/imagetools.h"
#include "mve/imagefile.h" // TMP

#include "defines.h"
#include "surf.h"

SFM_NAMESPACE_BEGIN


/* ---------------------------------------------------------------- */

void
Surf::process (void)
{
    this->keypoints.clear();
    this->octaves.clear();
    this->sat.reset();

    util::WallTimer timer;

    /* Desaturate input image. */
    if (this->orig->channels() != 1 && this->orig->channels() != 3)
        throw std::invalid_argument("Gray or color image expected");
    if (this->orig->channels() == 3)
        this->orig = mve::image::desaturate<uint8_t>(this->orig, mve::image::DESATURATE_LIGHTNESS);

    /* Build summed area table (SAT). */
    timer.reset();
    this->sat = mve::image::integral_image<uint8_t,SatType>(this->orig);
    std::cout << "Creating SAT image took "
        << timer.get_elapsed() << " ms." << std::endl;

    /* Compute Hessian response maps and find SS maxima (SURF 3.3). */
    timer.reset();
    this->create_octaves();
    std::cout << "Creating octaves took "
        << timer.get_elapsed() << " ms." << std::endl;

    /* Detect local extrema in the SS of Hessian response maps. */
    timer.reset();
    this->extrema_detection();
    std::cout << "Extrema detection took "
        << timer.get_elapsed() << " ms." << std::endl;

#if 1
    {
        std::cout << "Keypoints: " << this->keypoints.size() << ", per octave:";
        std::vector<int> keypoints_per_octave(this->octaves.size(), 0);
        for (std::size_t i = 0; i < this->keypoints.size(); ++i)
            keypoints_per_octave[this->keypoints[i].octave] += 1;
        for (std::size_t i = 0; i < this->octaves.size(); ++i)
            std::cout << " " << i << ":" << keypoints_per_octave[i];
         std::cout << std::endl;
    }
#endif

    /* Sub-pixel keypoint localization and filtering of weak keypoints. */
    timer.reset();
    this->keypoint_localization_and_filtering();
    std::cout << "Localization and filtering took "
        << timer.get_elapsed() << " ms." << std::endl;


#if 1
    {
        std::cout << "Keypoints: " << this->keypoints.size() << ", per octave:";
        std::vector<int> keypoints_per_octave(this->octaves.size(), 0);
        for (std::size_t i = 0; i < this->keypoints.size(); ++i)
            keypoints_per_octave[this->keypoints[i].octave] += 1;
        for (std::size_t i = 0; i < this->octaves.size(); ++i)
            std::cout << " " << i << ":" << keypoints_per_octave[i];
         std::cout << std::endl;
    }
#endif
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

    /* Kernel sizes per octave (given in 1/3 of the full size). */
    int const kernel[4][4] =
    {
        {  3,  5,  7,  9 },  // 9 15 21 27
        {  5,  9, 13, 17 },  // 15 27 39 51
        {  9, 17, 25, 33 },  // 27 51 75 99
        { 17, 33, 49, 65 }   // 51 99 147 195
    };

    /* Filter size. The actual kernel side length is 3 * fs. */
    int const fs = kernel[o][k];
    /* The sample spacing for the octaves. */
    int const step = math::algo::fastpow(2, o);
    /* Weight to balance between real gaussian kernel and approximated one. */
    RespType const weight = 0.912;  // See SURF 3.3 (4).
    /* The kernel width in pixel is fs * 3. Compute inverse kernel area. */
    RespType const inv_karea = 1.0 / (fs * fs * 3 * 3);

    /* Original dimensions and octave dimensions. */
    int const w = this->orig->width();
    int const h = this->orig->height();
    int ow = w;
    int oh = h;
    for (int i = 0; i < o; ++i)
    {
        ow = (ow + 1) >> 1;
        oh = (oh + 1) >> 1;
    }

    /* Generate the response maps. */
    SurfOctave::RespImage::Ptr img = SurfOctave::RespImage::create(ow, oh, 1);
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
            img->at(i) = dxx_t * dyy_t - weight * dxy_t * dxy_t;
        }

#if 0
    std::cout << "Saving response map " << o << "." << k << std::endl;
    mve::ByteImage::Ptr outimg = mve::ByteImage::create(ow, oh, 1);
    for (int i = 0; i < ow * oh; ++i)
        outimg->at(i) = math::algo::clamp(img->at(i) * 0.5, -128.0, 127.0) + 128;
    std::stringstream ss;
    ss << "/tmp/response-after/response-" << o << "-" << k << ".png";
    mve::image::save_file(outimg, ss.str());
#endif

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
            SurfOctave::RespImage::ConstPtr ss = this->octaves[o].imgs[s];
            SurfOctave::RespType const* ptr = &ss->at(0);
            for (int y = 0; y < height; ++y, ptr += width)
                for (int x = 1; x + 1 < width; ++x)
                    if (ptr[x] > ptr[x-1] && ptr[x] > ptr[x + 1])
                        this->check_maximum(o, s, x, y);
                    else if (ptr[x] < ptr[x-1] && ptr[x] < ptr[x + 1])
                        this->check_maximum(o, s, x, y);
        }
    }
}

/* ---------------------------------------------------------------- */

void
Surf::check_maximum(int o, int s, int x, int y)
{
    // Assumes that given coordinates are within bounds and a 1 pixel
    // boundary for comarisons in x, y and s direction.
    int const w = this->octaves[o].imgs[s]->width();
    int off1[8] = { -w - 1, -w, -w + 1, -1,    +1, w - 1, w, w + 1 };
    int off2[9] = { -w - 1, -w, -w + 1, -1, 0, +1, w - 1, w, w + 1 };

    SurfOctave::RespType const* ptr;
    int const off = x + y * w;

    bool largest = true;
    bool smallest = true;

    // Perform NMS check on the candidate sample first.
    ptr = this->octaves[o].imgs[s]->get_data_pointer() + off;
    SurfOctave::RespType value = ptr[0];
    for (int i = 0; (largest || smallest) && i < 8; ++i)
    {
        largest &= value > ptr[off1[i]];
        smallest &= value < ptr[off1[i]];
    }
    // Perform NMS check on the above sample.
    ptr = this->octaves[o].imgs[s - 1]->get_data_pointer() + off;
    for (int i = 0; (largest || smallest) && i < 9; ++i)
    {
        largest &= value > ptr[off2[i]];
        smallest &= value < ptr[off2[i]];
    }
    // Perform NMS check on the below sample.
    ptr = this->octaves[o].imgs[s + 1]->get_data_pointer() + off;
    for (int i = 0; (largest || smallest) && i < 9; ++i)
    {
        largest &= value > ptr[off2[i]];
        smallest &= value < ptr[off2[i]];
    }

    if (!largest && !smallest)
        return;

    // Seems like we found a keypoint.
    SurfKeypoint kp;
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
    SurfKeypoints filtered_keypoints;
    filtered_keypoints.reserve(this->keypoints.size());
    for (std::size_t i = 0; i < this->keypoints.size(); ++i)
    {
        SurfKeypoint kp = this->keypoints[i];
        bool solve_good = this->keypoint_localization(&kp);
        if (!solve_good)
            continue;
        filtered_keypoints.push_back(kp);
    }
    std::swap(this->keypoints, filtered_keypoints);
}

/* ---------------------------------------------------------------- */

bool
Surf::keypoint_localization (SurfKeypoint* kp)
{
    int const sample = static_cast<int>(kp->sample);
    int const w = this->octaves[kp->octave].imgs[sample]->width();
    int const h = this->octaves[kp->octave].imgs[sample]->height();
    int off = static_cast<int>(kp->x) + static_cast<int>(kp->y) * w;
    SurfOctave::RespType const *s0, *s1, *s2;
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

    if (std::abs(vec_x[0]) > 1.0f || std::abs(vec_x[1]) > 1.0f || std::abs(vec_x[2]) > 1.0f)
    {
        //std::cout <<"Large: " << vec_x << std::endl;
        return false;
    }

    /* Update keypoint. */
    float sampling = static_cast<float>(math::algo::fastpow(2, kp->octave));
    kp->x = (kp->x + vec_x[0]) * sampling;
    kp->y = (kp->y + vec_x[1]) * sampling;
    kp->sample += vec_x[2];

    if (kp->x < 0.0f || kp->x + 1.0f > this->orig->width()
        || kp->y < 0.0f || kp->y + 1.0f > this->orig->height())
    {
        //std::cout << "Rejection keypoint: OOB " << kp->x << " " << kp->y  << " / " << vec_x[0] << " " << vec_x[1] << std::endl;
        return false;
    }

    /* Compute actual DoG value at accurate keypoint x. */
    float val = N9[1][4] - 0.5 * vec_b.dot(vec_x);
    if (std::abs(val) < this->contrast_thres)
    {
        //std::cout << "Rejection keypoint: Contrast Thres" << std::endl;
        return false;
    }

    return true;
}

SFM_NAMESPACE_END
