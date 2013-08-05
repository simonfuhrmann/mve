#include <iostream>
#include <fstream>
#include <cstring>
#include <cerrno>

#include "util/timer.h"
#include "math/matrix.h"
#include "math/matrixtools.h"
#include "mve/imagefile.h"
#include "mve/imagetools.h"
#include "mve/sift.h"

MVE_NAMESPACE_BEGIN

void
Sift::process (void)
{
    /*
     * Creates the scale space representation of the image by
     * sampling the scale space and computing the DoG images.
     * See Section 3, 3.2 and 3.3 in SIFT article.
     */
    std::cout << "Creating " << (this->max_octave - this->min_octave)
        << " octaves (" << this->min_octave << " to " << this->max_octave
        << ")..." << std::endl;
    util::ClockTimer timer;
    this->create_octaves();
    std::cout << "Creating octaves took " << timer.get_elapsed() << std::endl;

    /*
     * Detects local extrema in the DoG function as described in Section 3.1.
     */
    std::cout << "Detecting local extrema..." << std::endl;
    timer.reset();
    this->extrema_detection();
    std::cout << "Detected " << this->keypoints.size() << " keypoints, took "
        << timer.get_elapsed() << std::endl;

#if 1
    /*
     * Accurate keypoint localization and filtering.
     * According to Section 4 in SIFT article.
     */
    std::cout << "Localizing and filtering keypoints..." << std::endl;
    timer.reset();
    this->keypoint_localization();
    std::cout << "Retained " << this->keypoints.size() << " stable "
        << "keypoints, took " << timer.get_elapsed() << "ms." << std::endl;
#endif

#if 0
    /*
     * Dump debug images to disc.
     */
    std::cout << "Dumping debug images to disc..." << std::endl;
    timer.reset();
    this->dump_octaves();
    std::cout << "Dumping images took " << timer.get_elapsed() << std::endl;
#endif

    /*
     * Difference of Gaussian images are not needed anymore.
     */
    for (std::size_t i = 0; i < this->octaves.size(); ++i)
        this->octaves[i].dog.clear();

    /*
     * Generate the list of keypoint descriptors.
     * See Section 5 and 6 in the SIFT article.
     * This list can in general be larger than the amount of keypoints,
     * since for each keypoint several descriptors may be created.
     */
    std::cout << "Generating keypoint descriptors..." << std::endl;
    timer.reset();
    this->descriptor_generation();
    std::cout << "Generated " << this->descriptors.size() << " descriptors "
        << "from " << this->keypoints.size() << " keypoints, took "
        << timer.get_elapsed() << "ms." << std::endl;

    /* Free memory. */
    this->octaves.clear();
}

/* ---------------------------------------------------------------- */

void
Sift::set_image (mve::ByteImage::ConstPtr img)
{
    if (img->channels() != 1 && img->channels() != 3)
        throw std::invalid_argument("Gray or color image expected");

    this->orig = mve::image::byte_to_float_image(img);
    if (img->channels() == 3)
    {
        this->orig = mve::image::desaturate<float>
            (this->orig, mve::image::DESATURATE_AVERAGE);
    }
}

/* ---------------------------------------------------------------- */

void
Sift::set_float_image (mve::FloatImage::ConstPtr img)
{
    if (img->channels() != 1 && img->channels() != 3)
        throw std::invalid_argument("Gray or color image expected");

    if (img->channels() == 3)
    {
        this->orig = mve::image::desaturate<float>
            (img, mve::image::DESATURATE_AVERAGE);
    }
    else
    {
        this->orig = img->duplicate();
    }
}

/* ---------------------------------------------------------------- */

void
Sift::create_octaves (void)
{
    this->octaves.clear();

    /*
     * Create octave -1. The original image is assumed to have blur
     * sigma = 0.5. The double size image therefore has sigma = 1.
     */
    if (this->min_octave < 0)
    {
        //std::cout << "Creating octave -1..." << std::endl;
        mve::FloatImage::Ptr img
            = mve::image::rescale_double_size_supersample<float>(this->orig);
        this->add_octave(img, this->inherent_blur * 2.0f, this->pre_smoothing);
    }

    /*
     * Prepare image for the first positive octave by downsampling.
     * This code is executed only if min_octave > 0.
     */
    mve::FloatImage::ConstPtr img = this->orig;
    for (int i = 0; i < this->min_octave; ++i)
    {
        std::cout << "Downsampling image for octave "
            << (i + 1) << "..." << std::endl;
        img = mve::image::rescale_half_size_gaussian<float>(img);
    }

    /*
     * Create new octave from 'img', then subsample octave image where
     * sigma is doubled to get a new base image for the next octave.
     */
    float img_sigma = this->inherent_blur;
    for (int i = std::max(0, this->min_octave); i <= this->max_octave; ++i)
    {
        //std::cout << "Creating octave " << i << "..." << std::endl;
        this->add_octave(img, img_sigma, this->pre_smoothing);
        //SiftOctave const& oct(this->octaves.back());
        //mve::FloatImage::ConstPtr prev(oct.img[this->octave_samples]);
        //img = mve::image::rescale_half_size_subsample<float>(prev);
        img = mve::image::rescale_half_size_gaussian<float>(img);
        img_sigma = this->pre_smoothing;
    }
}

/* ---------------------------------------------------------------- */

void
Sift::add_octave (mve::FloatImage::ConstPtr image,
        float has_sigma, float target_sigma)
{
    /*
     * First, bring the provided image to the target blur.
     * Since L * g(sigma1) * g(sigma2) = L * g(sqrt(sigma1^2 + sigma2^2)),
     * we need to blur with sigma = sqrt(target_sigma^2 - has_sigma^2).
     */
    float sigma = std::sqrt(MATH_POW2(target_sigma) - MATH_POW2(has_sigma));
    //std::cout << "Pre-blurring image to sigma " << target_sigma << " (has "
    //    << has_sigma << ", blur = " << sigma << ")..." << std::endl;
    mve::FloatImage::Ptr base = (target_sigma > has_sigma
        ? mve::image::blur_gaussian<float>(image, sigma)
        : mve::FloatImage::Ptr(image->duplicate()));

    /* Create the new octave and add initial image. */
    this->octaves.push_back(SiftOctave());
    SiftOctave& oct = this->octaves.back();
    oct.img.push_back(base);

    /* 'k' is the constant factor between the scales in scale space. */
    float k = std::pow(2.0f, 1.0f / (float)this->octave_samples);
    sigma = target_sigma;

    /* Create other (s+2) samples of the octave to get a total of (s+3). */
    for (int i = 1; i < this->octave_samples + 3; ++i)
    {
        /* Calculate the blur sigma the image will get. */
        float sigmak = sigma * k;
        float blur_sigma = std::sqrt(MATH_POW2(sigmak) - MATH_POW2(sigma));

        /* Blur the image to create a new scale space sample. */
        //std::cout << "Blurring image to sigma " << sigmak << " (has " << sigma
        //    << ", blur = " << blur_sigma << ")..." << std::endl;
        mve::FloatImage::Ptr img = mve::image::blur_gaussian<float>
            (base, blur_sigma);
        oct.img.push_back(img);

        /* Create the Difference of Gaussian image (DoG). */
        mve::FloatImage::Ptr dog = mve::image::subtract<float>(img, base);
        oct.dog.push_back(dog);

        /* Update previous image and sigma for next round. */
        base = img;
        sigma = sigmak;
    }
}

/* ---------------------------------------------------------------- */

void
Sift::extrema_detection (void)
{
    /* Delete previous keypoints. */
    this->keypoints.clear();

    /* Detect keypoints in each octave... */
    for (std::size_t i = 0; i < this->octaves.size(); ++i)
    {
        SiftOctave const& oct(this->octaves[i]);
        /* In each octave, take three subsequent DoG images and detect. */
        for (int s = 0; s < (int)oct.dog.size() - 2; ++s)
        {
            mve::FloatImage::ConstPtr samples[3] =
            { oct.dog[s + 0], oct.dog[s + 1], oct.dog[s + 2] };
            //std::size_t num =
            this->extrema_detection(samples, (int)i + this->min_octave, s);
            //std::cout << "Detected " << num << " extrema in octave "
            //    << i << ", sample " << s << "..." << std::endl;
        }
    }
}

/* ---------------------------------------------------------------- */

std::size_t
Sift::extrema_detection (mve::FloatImage::ConstPtr s[3], int oi, int si)
{
    int const w = s[1]->width();
    int const h = s[1]->height();

    /* Offsets for the 9-neighborhood w.r.t. center pixel. */
    int noff[9] = { -1 - w, 0 - w, 1 - w, -1, 0, 1, -1 + w, 0 + w, 1 + w };

    /*
     * Iterate over all pixels in s[1], and check if pixel is maximum
     * (or minumum) in its 27-neighborhood.
     */
    std::size_t detected = 0;
    std::size_t off = w;
    for (int y = 1; y < h - 1; ++y, off += w)
        for (int x = 1; x < w - 1; ++x)
        {
            std::size_t idx = off + x;

            bool largest = true;
            bool smallest = true;
            float center_value = s[1]->at(idx);
            for (int l = 0; (largest || smallest) && l < 3; ++l)
                for (int i = 0; (largest || smallest) && i < 9; ++i)
                {
                    if (l == 1 && i == 4) // Skip center pixel
                        continue;
                    if (s[l]->at(idx + noff[i]) >= center_value)
                        largest = false;
                    if (s[l]->at(idx + noff[i]) <= center_value)
                        smallest = false;
                }

            /* Skip non-maximum values. */
            if (!smallest && !largest)
                continue;

            /* Yummy. Add detected scale space extremum. */
            SiftKeypoint kp;
            kp.o = oi;
            kp.ix = x;
            kp.iy = y;
            kp.is = si;
            kp.x = static_cast<float>(kp.ix);
            kp.y = static_cast<float>(kp.iy);
            kp.s = static_cast<float>(kp.is);
            //kp.scale = this->keypoint_absolute_scale(kp);
            this->keypoints.push_back(kp);
            detected += 1;
        }

    return detected;
}

/* ---------------------------------------------------------------- */

void
Sift::keypoint_localization (void)
{
    /*
     * Iterate over all keypoints, accurately localize minima and maxima
     * in the DoG function by fitting a quadratic Taylor polynomial
     * around the keypoint.
     */

    std::size_t num_singular = 0;
    std::size_t witer = 0; // Write iterator
    for (std::size_t i = 0; i < this->keypoints.size(); ++i)
    {
        /* Copy keypoint. */
        SiftKeypoint kp(this->keypoints[i]);

        /* Get corresponding octave and DoG images. */
        SiftOctave const& oct(this->octaves[kp.o - this->min_octave]);
        mve::FloatImage::ConstPtr dogs[3] =
        {
            oct.dog[kp.is + 0], oct.dog[kp.is + 1], oct.dog[kp.is + 2]
        };

        /* Shorthand for image width and height. */
        int const w = dogs[0]->width();
        int const h = dogs[0]->height();
        /* The integer and floating point location of the keypoints. */
        int ix = kp.ix;
        int iy = kp.iy;
        float fx, fy, fs;
        /* The first and second order derivatives. */
        float Dx, Dy, Ds;
        float Dxx, Dyy, Dss;
        float Dxy, Dxs, Dys;

        /*
         * Locate the keypoint using second order Taylor approximation.
         * The procedure might get iterated around a neighboring pixel if
         * the accurate keypoint is off by >0.6 from the center pixel.
         */
#       define AT(S,OFF) (dogs[S]->at(px + OFF))
        for (int j = 0; j < 5; ++j)
        {
            std::size_t px = iy * w + ix;

            /* Compute first and second derivatives. */
            Dx = (AT(1,1) - AT(1,-1)) * 0.5f;
            Dy = (AT(1,w) - AT(1,-w)) * 0.5f;
            Ds = (AT(2,0) - AT(0,0))  * 0.5f;

            Dxx = AT(1,1) + AT(1,-1) - 2.0f * AT(1,0);
            Dyy = AT(1,w) + AT(1,-w) - 2.0f * AT(1,0);
            Dss = AT(2,0) + AT(0,0)  - 2.0f * AT(1,0);

            Dxy = (AT(1,1+w) + AT(1,-1-w) - AT(1,-1+w) - AT(1,1-w)) * 0.25f;
            Dxs = (AT(2,1)   + AT(0,-1)   - AT(2,-1)   - AT(0,1))   * 0.25f;
            Dys = (AT(2,w)   + AT(0,-w)   - AT(2,-w)   - AT(0,w))   * 0.25f;

            /* Setup the Hessian matrix. */
            math::Matrix3f A;
            A[0] = Dxx; A[1] = Dxy; A[2] = Dxs;
            A[3] = Dxy; A[4] = Dyy; A[5] = Dys;
            A[6] = Dxs; A[7] = Dys; A[8] = Dss;

            /* Compute determinant to detect singular matrix. */
            float detA = math::matrix_determinant(A);
            if (MATH_EPSILON_EQ(detA, 0.0f, 1e-15f))
            {
                num_singular += 1;
                fx = fy = fs = 0.0f; // FIXME: Handle this case?
                break;
            }

            /* Invert the matrix to get the accurate keypoint. */
            A = math::matrix_inverse(A, detA);
            math::Vec3f b(-Dx, -Dy, -Ds);
            b = A * b;
            fx = b[0]; fy = b[1]; fs = b[2];

            /* Check if accurate location is far away from pixel center. */
            int dx = (fx > 0.6f && ix < w-2) * 1 + (fx < -0.6f && ix > 1) * -1;
            int dy = (fy > 0.6f && iy < h-2) * 1 + (fy < -0.6f && iy > 1) * -1;

            /* If the accurate location is closer to another pixel,
             * repeat localization around the other pixel. */
            if (dx != 0 || dy != 0)
            {
                ix += dx;
                iy += dy;
                continue;
            }

            /* Accurate location looks good. */
            break;
        }

        /* Calcualte function value D(x) at accurate keypoint x. */
        float val = dogs[1]->at(ix, iy, 0) + 0.5f * (Dx * fx + Dy * fy + Ds * fs);

        /* Calcualte edge response score Tr(H)^2 / Det(H), see Section 4.1. */
        float hessian_trace = Dxx + Dyy;
        float hessian_det = Dxx * Dyy - MATH_POW2(Dxy);
        float hessian_score = MATH_POW2(hessian_trace) / hessian_det;
        float score_thres = MATH_POW2(this->edge_ratio_thres + 1.0f)
            / this->edge_ratio_thres;

        /*
         * Set accurate final keypoint location.
         */
        kp.ix = ix;
        kp.iy = iy;
        kp.x = (float)ix + fx;
        kp.y = (float)iy + fy;
        kp.s = (float)kp.is + fs;
        kp.scale = this->keypoint_absolute_scale(kp);

        // FIXME: since fx,fy are allowed to be larger than 0.5, kp.x
        // is NOT a proper rounded versino of kp.ix.

        /*
         * Discard keypoints with:
         * 1. low contrast (value of DoG function at keypoint),
         * 2. negative hessian determinant (curvatures with different sign),
         *    Note that negative score implies negative determinant.
         * 3. large edge response (large hessian score),
         * 4. unstable keypoint accurate locations,
         * 5. keypoints beyond the scale space boundary.
         */
        if (std::abs(val) < this->contrast_thres
            || hessian_score < 0.0f || hessian_score > score_thres
            || std::abs(fx) > 1.5f || std::abs(fy) > 1.5f || std::abs(fs) > 1.0f
            || kp.s < -1.0f || kp.s > (float)(this->octave_samples)
            || kp.x < 0.0f || kp.x > (float)(w - 1)
            || kp.y < 0.0f || kp.y > (float)(h - 1))
        {
            //std::cout << " REJECTED!" << std::endl;
            continue;
        }
        else
        {
            //std::cout << std::endl;
        }

        /* Keypoint is accepted, copy to write iter and advance. */
        this->keypoints[witer] = kp;
        witer += 1;
    }

    /* Limit vector size to accepted keypoints. */
    this->keypoints.resize(witer);

    if (num_singular)
    {
        std::cout << "Warning: " << num_singular
            << " singular matrices detected!" << std::endl;
    }
}

/* ---------------------------------------------------------------- */

void
Sift::descriptor_generation (void)
{
    if (this->octaves.empty())
        throw std::runtime_error("Octaves not available!");

    this->descriptors.clear();
    this->descriptors.reserve(this->keypoints.size() * 3 / 2);

    /*
     * Keep a buffer of S+3 gradient and orientation images for the current
     * octave. Once the octave is changed, these images are recomputed.
     * To ensure efficiency, the octave index must always increase,
     * never decreased again, which is enforced during the algorithm.
     */
    int octave_index = this->min_octave - 1;
    SiftOctave* octave = NULL;

    /* Walk over all keypoints and compute descriptors. */
    for (std::size_t i = 0; i < this->keypoints.size(); ++i)
    {
        SiftKeypoint const& kp(this->keypoints[i]);

        /* Generate new gradient and orientation images if octave changed. */
        if (kp.o < octave_index)
            throw std::runtime_error("Decreasing octave index!");
        if (kp.o > octave_index)
        {
            octave_index = kp.o;
            if (octave)
            {
                octave->grad.clear();
                octave->ori.clear();
            }
            octave = &this->octaves[octave_index - this->min_octave];
            this->generate_grad_ori_images(octave);
        }

        /* Orientation assignment. This returns multiple orientations. */
        std::vector<float> orientations;
        orientations.reserve(8);
        this->orientation_assignment(kp, octave, orientations);

        /* Feature vector extraction. */
        for (std::size_t j = 0; j < orientations.size(); ++j)
        {
            SiftDescriptor desc;
            desc.k = kp;
            desc.orientation = orientations[j];
            this->descriptor_assignment(desc, octave);
            this->descriptors.push_back(desc);
        }
    }
}

/* ---------------------------------------------------------------- */

void
Sift::generate_grad_ori_images (SiftOctave* octave)
{
    octave->grad.clear();
    octave->grad.reserve(octave->img.size());
    octave->ori.clear();
    octave->ori.reserve(octave->img.size());

    std::size_t const w = octave->img[0]->width();
    std::size_t const h = octave->img[0]->height();

    //std::cout << "Generating gradient and orientation images..." << std::endl;
    for (std::size_t i = 0; i < octave->img.size(); ++i)
    {
        mve::FloatImage::ConstPtr img(octave->img[i]);
        mve::FloatImage::Ptr grad(mve::FloatImage::create(w, h, 1));
        mve::FloatImage::Ptr ori(mve::FloatImage::create(w, h, 1));

        std::size_t iiter = w + 1;
        for (std::size_t y = 1; y < h - 1; ++y, iiter += 2)
            for (std::size_t x = 1; x < w - 1; ++x, ++iiter)
            {
                float m1x = img->at(iiter - 1);
                float p1x = img->at(iiter + 1);
                float m1y = img->at(iiter - w);
                float p1y = img->at(iiter + w);
                float dx = 0.5f * (p1x - m1x); // Does the 0.5 make any difference?
                float dy = 0.5f * (p1y - m1y);

                float atan2f = std::atan2(dy, dx);
                grad->at(iiter) = std::sqrt(dx * dx + dy * dy);
                ori->at(iiter) = atan2f < 0.0f ? atan2f + MATH_PI * 2.0f : atan2f;
            }
        octave->grad.push_back(grad);
        octave->ori.push_back(ori);

#if 0
        static int num = -1;
        num++;
        {
            std::stringstream ss;
            ss << "/tmp/grad-num-" << num << ".png";
            mve::image::save_file(mve::image::float_to_byte_image(grad, 0.0f, 0.3f), ss.str());
        }
        {
            std::stringstream ss;
            ss << "/tmp/ori-num" << num << ".png";
            mve::image::save_file(mve::image::float_to_byte_image(ori, 0, 2.0f * MATH_PI), ss.str());
        }
#endif
    }
}

/* ---------------------------------------------------------------- */

void
Sift::orientation_assignment (SiftKeypoint const& kp,
    SiftOctave const* octave, std::vector<float>& orientations)
{
    int const nbins = 36;
    float const nbinsf = static_cast<float>(nbins);

    /* Prepare 36-bin histogram. */
    float hist[nbins];
    std::fill(hist, hist + nbins, 0.0f);

    /* Integral x and y coordinates and closest scale sample. */
    int ix = static_cast<int>(kp.x + 0.5f);
    int iy = static_cast<int>(kp.y + 0.5f);
    int is = static_cast<int>(math::algo::round(kp.s));
    float sigma = this->keypoint_relative_scale(kp);

    //if (is < -1 || is > this->octave_samples)
    //    throw std::runtime_error("SIFT internal: scale index out of bounds. clamp necessary!");

    /* Images with its dimension for the keypoint. */
    mve::FloatImage::ConstPtr grad(octave->grad[is + 1]);
    mve::FloatImage::ConstPtr ori(octave->ori[is + 1]);
    int const w = grad->width();
    int const h = grad->height();

    /*
     * Compute window size w, the full window has 2*w+1 pixel.
     * The factor 3 makes the window large enough such that the gaussian
     * has very little weight beyond the window. The value 1.5 is from
     * the SIFT paper.
     */
    float const sigma_factor = 1.5f;
    int win = static_cast<int>(sigma * sigma_factor * 3.0f);
    int center = iy * w + ix;
    float dxf = kp.x - static_cast<float>(ix);
    float dyf = kp.y - static_cast<float>(iy);
    float maxdist = static_cast<float>(win*win) + 0.5f;

    /* Populate histogram over window, intersected with (1,1), (w-2,h-2). */
    int dimx[2] = { std::max(-win, 1 - ix), std::min(win, w - ix - 2) };
    int dimy[2] = { std::max(-win, 1 - iy), std::min(win, h - iy - 2) };
    for (int dy = dimy[0]; dy <= dimy[1]; ++dy)
    {
        int yoff = dy * w;
        for (int dx = dimx[0]; dx <= dimx[1]; ++dx)
        {
            /* Limit to circular window (centered at accurate keypoint). */
            float dist = MATH_POW2(dx-dxf) + MATH_POW2(dy-dyf);
            if (dist > maxdist)
                continue;

            float gm = grad->at(center + yoff + dx); // gradient magnitude
            float go = ori->at(center + yoff + dx); // gradient orientation
            float w = math::algo::gaussian_xx(dist, sigma * sigma_factor);
            int bin = static_cast<int>(nbinsf * go / (2.0f * MATH_PI));
            bin = math::algo::clamp(bin, 0, nbins - 1);
            hist[bin] += gm * w;
        }
    }

    /* Smooth histogram. */
    for (int i = 0; i < 6; ++i)
    {
        float first = hist[0];
        float prev = hist[nbins - 1];
        for (int j = 0; j < nbins - 1; ++j)
        {
            float current = hist[j];
            hist[j] = (prev + current + hist[j + 1]) / 3.0f;
            prev = current;
        }
        hist[nbins - 1] = (prev + hist[nbins - 1] + first) / 3.0f;
    }

    /* Find maximum element. */
    float maxh = *std::max_element(hist, hist + nbins);

    /* Find peaks within 80% of max element. */
    for (int i = 0; i < nbins; ++i)
    {
        float h0 = hist[(i + nbins - 1) % nbins];
        float h1 = hist[i];
        float h2 = hist[(i + 1) % nbins];

        /* These peaks must be a local maximum! */
        if (h1 <= 0.8f * maxh || h1 <= h0 || h1 <= h2)
            continue;

        /*
         * Quadratic interpolation to find accurate maximum.
         * f(x) = ax^2 + bx + c, f(-1) = h0, f(0) = h1, f(1) = h2
         * --> a = 1/2 (h0 - 2h1 + h2), b = 1/2 (h2 - h0), c = h1.
         * x = f'(x) = 2ax + b = 0 --> x = -1/2 * (h2 - h0) / (h0 - 2h1 + h2)
         */
        float x = -0.5f * (h2 - h0) / (h0 - 2.0f * h1 + h2);
        float o =  2.0f * MATH_PI * (x + (float)i + 0.5f) / nbinsf;
        orientations.push_back(o);
    }
}

/* ---------------------------------------------------------------- */

void
Sift::descriptor_assignment (SiftDescriptor& desc, SiftOctave* octave)
{
    /*
     * The final feature vector has size PXB * PXB * OHB.
     * The following constants should not be changed yet, as the
     * (PXB^2 * OHB = 128) element feature vector is still hard-coded.
     */
    //int const PIX = 16; // Descriptor region with 16x16 pixel
    int const PXB = 4; // Pixel bins with 4x4 bins
    int const OHB = 8; // Orientation histogram with 8 bins

    SiftKeypoint const& kp(desc.k);

    /* Integral x and y coordinates and closest scale sample. */
    int ix = static_cast<int>(kp.x + 0.5f);
    int iy = static_cast<int>(kp.y + 0.5f);
    int is = static_cast<int>(math::algo::round(kp.s)); // kp.s can be neg.
//    int is = static_cast<int>(math::algo::round(0.5f)); // kp.s can be neg.
    float dxf = kp.x - static_cast<float>(ix);
    float dyf = kp.y - static_cast<float>(iy);
    float sigma = this->keypoint_relative_scale(kp);

    /* Images with its dimension for the keypoint. */
    mve::FloatImage::ConstPtr grad(octave->grad[is + 1]);
    mve::FloatImage::ConstPtr ori(octave->ori[is + 1]);
    int const w = grad->width();
    int const h = grad->height();

    /* Clear feature vector. */
    desc.vec.fill(0.0f);

#if 1
    /* Rotation constants given by descriptor orientation. */
    float const sino = std::sin(desc.orientation);
    float const coso = std::cos(desc.orientation);

    /*
     * Compute window size.
     * Each spacial bin has an extension of 3 * sigma (sigma is the scale
     * of the keypoint). For interpolation we need another half bin at
     * both ends in each dimension. And since the window can be arbitrarily
     * rotated, we need to multiply with sqrt(2). The window size is:
     * 2W = sqrt(2) * 3 * sigma * (PXB + 1).
     */
    float binsize = 3.0f * sigma;
    int win = std::sqrt(2.0f) * binsize * (float)(PXB + 1) * 0.5f;

    /*
     * Iterate over the window, intersected with the image region
     * from (1,1) to (w-2, h-2) since gradients/orientations are
     * not defined at the boundary pixels. Add all samples to the
     * corresponding bin.
     */
    int center = iy * w + ix; // Center pixel at KP location
    int dimx[2] = { std::max(-win, 1 - ix), std::min(win, w - ix - 2) };
    int dimy[2] = { std::max(-win, 1 - iy), std::min(win, h - iy - 2) };
    for (int dy = dimy[0]; dy <= dimy[1]; ++dy)
    {
        int yoff = dy * w;
        for (int dx = dimx[0]; dx <= dimx[1]; ++dx)
        {
            /* Get pixel gradient magnitude and orientation. */
            float mod = grad->at(center + yoff + dx);
            float angle = ori->at(center + yoff + dx);
            float theta = angle - desc.orientation;
            if (theta < 0.0f)
                theta += 2.0f * MATH_PI;

            /* Compute fractional coordinates w.r.t. the window. */
            float winx = (float)dx - dxf;
            float winy = (float)dy - dyf;

            /*
             * Compute normalized coordinates w.r.t. bins. The window
             * coordinates are rotated around the keypoint. The bins are
             * chosen such that 0 is the coordinate of the first bins center
             * in each dimension. In other words, (0,0,0) is the coordinate
             * of the first bin center in the three dimensional histogram.
             */
            float binoff = (float)(PXB - 1) / 2.0f;
            float binx = (coso * winx + sino * winy) / binsize + binoff;
            float biny = (-sino * winx + coso * winy) / binsize + binoff;
            float bint = theta * (float)OHB / (2.0f * MATH_PI) - 0.5f;

            /* Compute circular window weight for the sample. */
            float gaussian_sigma = 0.5f * (float)PXB;
            float gaussian_weight = math::algo::gaussian_xx
                (MATH_POW2(binx - binoff) + MATH_POW2(biny - binoff),
                gaussian_sigma);

            /* Total contribution of the sample in the histogram is now: */
            float contrib = mod * gaussian_weight;

            /*
             * Distribute values into bins (using trilinear interpolation).
             * Each sample is inserted into 8 bins. Some of these bins may
             * not exist, because the sample is outside the keypoint window.
             */
            int bxi[2] = { (int)std::floor(binx), (int)std::floor(binx) + 1 };
            int byi[2] = { (int)std::floor(biny), (int)std::floor(biny) + 1 };
            int bti[2] = { (int)std::floor(bint), (int)std::floor(bint) + 1 };

            float w[3][2] = {
                { (float)bxi[1] - binx, 1.0f - ((float)bxi[1] - binx) },
                { (float)byi[1] - biny, 1.0f - ((float)byi[1] - biny) },
                { (float)bti[1] - bint, 1.0f - ((float)bti[1] - bint) }
            };

            // Wrap around orientation histogram
            if (bti[0] < 0)
                bti[0] += OHB;
            if (bti[1] >= OHB)
                bti[1] -= OHB;

            /* Iterate the 8 bins and add weighted contrib to each. */
            int const xstride = OHB;
            int const ystride = OHB * PXB;
            for (int y = 0; y < 2; ++y)
                for (int x = 0; x < 2; ++x)
                    for (int t = 0; t < 2; ++t)
                    {
                        if (bxi[x] < 0 || bxi[x] >= PXB
                            || byi[y] < 0 || byi[y] >= PXB)
                            continue;

                        int idx = bti[t] + bxi[x] * xstride + byi[y] * ystride;
                        desc.vec[idx] += contrib * w[0][x] * w[1][y] * w[2][t];
                    }
        }
    }
#endif

    /* Normalize the feature vector. */
    desc.vec.normalize();

    /* Truncate descriptor values to 0.2. */
    for (int i = 0; i < PXB * PXB * OHB; ++i)
        desc.vec[i] = std::min(desc.vec[i], 0.2f);

    /* Normalize once again. */
    desc.vec.normalize();
}

/* ---------------------------------------------------------------- */

/*
 * The scale of a keypoint is: scale = sigma0 * 2^(octave + (s+1)/S).
 * sigma0 is the initial blur (1.6), octave the octave index of the
 * keypoint (-1, 0, 1, ...) and scale space sample s in [-1,S+1] where
 * S is the amount of samples per octave. Since the initial blur 1.6
 * corresponds to scale space sample -1, we add 1 to the scale index.
 */

float
Sift::keypoint_relative_scale (SiftKeypoint const& kp)
{
    return this->pre_smoothing * std::pow(2.0f,
        (kp.s + 1.0f) / this->octave_samples);
}

float
Sift::keypoint_absolute_scale (SiftKeypoint const& kp)
{
    return this->pre_smoothing * std::pow(2.0f,
        kp.o + (kp.s + 1.0f) / this->octave_samples);
}

/* ---------------------------------------------------------------- */

void
Sift::write_keyfile (std::string const& filename)
{
    std::ofstream out(filename.c_str());
    if (!out.good())
        throw std::runtime_error(std::strerror(errno));

    if (this->descriptors.empty())
    {
        out << "0 0" << std::endl;
        out.close();
        return;
    }

    /* Write header. */
    {
        SiftDescriptor const& first = this->descriptors.front();
        out << this->descriptors.size() << " " << first.vec.dim() << std::endl;
    }

    /* Write all descriptors. */
    for (std::size_t i = 0; i < this->descriptors.size(); ++i)
    {
        SiftDescriptor const& desc(this->descriptors[i]);
        SiftKeypoint const& kp(desc.k);
        float factor = std::pow(2.0f, (float)kp.o);
        float kpx = factor * (kp.x + 0.5f) - 0.5f;
        float kpy = factor * (kp.y + 0.5f) - 0.5f;
        out << kpx << " " << kpy << " " << kp.scale
            << " " << desc.orientation << std::endl << "   ";
        for (int j = 0; j < 128; ++j)
            out << " " << (int)(desc.vec[j] * 255.0f);
        out << std::endl;
    }

    out.close();
}

/* ---------------------------------------------------------------- */

void
Sift::read_keyfile (std::string const& filename)
{
    std::ifstream in(filename.c_str());
    if (!in.good())
        throw std::runtime_error(std::strerror(errno));

    this->octaves.clear(); // we can't fill these
    this->keypoints.clear(); // we can only fill part of them
    this->descriptors.clear(); // should be able to fill all of them

    /* Read header. */
    {
        size_t numDescriptors, dim;
        in >> numDescriptors >> dim;
        this->descriptors.resize(numDescriptors);
    }

    /* Read all descriptors. */
    for (std::size_t i = 0; i < this->descriptors.size(); ++i)
    {
        SiftDescriptor desc;
        SiftKeypoint kp;
        float kpx, kpy;

        in >> kpx >> kpy >> kp.scale >> desc.orientation;

        kp.x = kpx;
        kp.y = kpy;
        kp.o = 0;
        kp.s = 0;

        int d[128];
        for (int j = 0; j < 128; ++j)
            in >> d[j];

        for (int j = 0; j < 128; ++j)
            desc.vec[j] = ((float)d[j])/255.0f;

        this->keypoints.push_back(kp);
        desc.k = kp;
        descriptors[i] = desc;
    }
    in.close();
}

/* ---------------------------------------------------------------- */

void
Sift::dump_octaves (void)
{
    std::cout << "Dumping images to /tmp ..." << std::endl;

    /* Dumps all octaves to disc, for debugging purposes. */
    for (std::size_t i = 0; i < this->octaves.size(); ++i)
    {
        SiftOctave const& oct(this->octaves[i]);
        for (std::size_t j = 0; j < oct.img.size(); ++j)
        {
            std::stringstream ss;
            ss << "/tmp/sift-octave_" << i << "-layer_" << j << ".png";
            mve::ByteImage::Ptr img = mve::image::float_to_byte_image(oct.img[j]);
            mve::image::save_file(img, ss.str());
        }

        for (std::size_t j = 0; j < oct.dog.size(); ++j)
        {
            std::stringstream ss;
            ss << "/tmp/sift-octave_" << i << "-dog_" << j << ".png";
            mve::ByteImage::Ptr img = mve::image::float_to_byte_image(oct.dog[j], -0.5f, 0.5f);
            mve::image::save_file(img, ss.str());
        }
    }
}

MVE_NAMESPACE_END
