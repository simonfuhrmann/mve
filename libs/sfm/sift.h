/*
 * SIFT implementation.
 * Written by Simon Fuhrmann with great help from Ronny Klowsky
 *
 * Notes:
 * - The implementation allows a minmum octave of -1 only
 * - The descriptor extration supports 128 dimensions only
 * - Coordinates in the keypoint are relative to the octave.
 *   Abosulte coordinates are obtained by (TODO why? explain):
 *   (x + 0.5, y + 0.5) * 2^octave - (0.5, 0.5).
 *
 *
 * TODO:
 * - Refactor Keypoint to only have floating point coordinate
 * - Refactor Descriptor to use std::vector
 * - Refactor Descriptor to use coordinates, no KP copy
 * - Move keypoint scale to descriptor
 * - Save memory by finding a more efficent code path to create octaves
 */
#ifndef SFM_SIFT_HEADER
#define SFM_SIFT_HEADER

#include <string>
#include <vector>

#include "math/vector.h"
#include "mve/image.h"

#include "defines.h"

SFM_NAMESPACE_BEGIN

/**
 * Implementation of the SIFT feature detector and descriptor.
 * The implementation follows the description of the journal article:
 *
 *   Distinctive Image Features from Scale-Invariant Keypoints,
 *   David G. Lowe, International Journal of Computer Vision, 2004.
 *
 * The implementation used the siftpp implementation as reference for some
 * parts of the algorithm. siftpp is available here:
 *
 *   http://www.vlfeat.org/~vedaldi/code/siftpp.html
 */
class Sift
{
public:
    /**
     * Representation of a SIFT keypoint.
     *
     * The keypoint locations are relative to the resampled size in
     * the image pyramid. To get the size relative to the input image,
     * each of (ix,iy,x,y) need to be multiplied with 2^o, where o
     * is the octave index of the keypoint. The octave index is -1 for the
     * upsampled image, 0 for the input image and >0 for subsampled images.
     * Note that the scale of the KP is already relative to the input image.
     */
    struct Keypoint
    {
        /** Octave index of the keypoint. Can be negative. */
        int octave;
        /** Sample index. Initally integer in {0 ... S-1}, later in [-1,S]. */
        float sample;
        /** Keypoint x-coordinate. Initially integer, later sub-pixel. */
        float x;
        /** Keypoint y-coordinate. Initially integer, later sub-pixel. */
        float y;
    };

    /**
     * Representation of the SIFT descriptor.
     * The descriptor is created in a rotation invariant way. The resulting
     * vector is unsigned and normalized, and has 128 dimensions.
     */
    struct Descriptor
    {
        float x;
        float y;
        float scale; ///< the scale (or sigma) of the keypoint
        float orientation; ///< Orientation of the KP in [0, 2PI]
        math::Vector<float, 128> data; ///< The feature vector
    };

public:
    typedef std::vector<Keypoint> Keypoints;
    typedef std::vector<Descriptor> Descriptors;

public:
    Sift (void);

    /** Sets the input image. */
    void set_image (mve::ByteImage::ConstPtr img);
    /** Sets the input image. */
    void set_float_image (mve::FloatImage::ConstPtr img);

    /**
     * Sets the amount of samples per octave. This defaults to 3
     * and results to 6 blurred images and 5 DoG images per octave.
     */
    void set_samples_per_octave (int samples);

    /**
     * Sets the amount of octaves by specifying the minimum octave
     * and the maximum octave. This defaults to -1 and 5. The minimum
     * allowed octave is -1.
     */
    void set_min_max_octave (int min_octave, int max_octave);

    /**
     * Sets contrast threshold, i.e. thresholds the absolute DoG value
     * at the accurately detected keypoint. Defaults to 0.02 / samples.
     */
    void set_contrast_threshold (float thres);

    /**
     * Sets the edge threshold to eliminate edge responses. The threshold is
     * the ratio between the principal curvatures (variable "r" in SIFT),
     * and defaults to 10.
     */
    void set_edge_threshold (float thres);

    /** Sets the inherent blur sigma in the input image. Default is 0.5. */
    void set_inherent_blur (float sigma);

    /** Sets the amount of pre-smoothing. Default sigma is 1.6. */
    void set_pre_smoothing (float sigma);

    /** Starts the SIFT keypoint detection and descriptor extraction. */
    void process (void);

    /** Returns the list of keypoints. */
    Keypoints const& get_keypoints (void) const;

    /** Returns the list of descriptors. */
    Descriptors const& get_descriptors (void) const;

protected:
    /**
     * Representation of a SIFT octave.
     */
    struct Octave
    {
        typedef std::vector<mve::FloatImage::Ptr> ImageVector;
        ImageVector img; ///< S+3 images per octave
        ImageVector dog; ///< S+2 difference of gaussian images
        ImageVector grad; ///< S+3 gradient images
        ImageVector ori; ///< S+3 orientation images
    };

protected:
    typedef std::vector<Octave> Octaves;

protected:
    void create_octaves (void);
    void add_octave (mve::FloatImage::ConstPtr image,
        float has_sigma, float target_sigma);
    void extrema_detection (void);
    std::size_t extrema_detection (mve::FloatImage::ConstPtr s[3],
        int oi, int si);
    void keypoint_localization (void);

    void descriptor_generation (void);
    void generate_grad_ori_images (Octave* octave);
    void orientation_assignment (Keypoint const& kp,
        Octave const* octave, std::vector<float>& orientations);
    void descriptor_assignment (Keypoint const& kp, Descriptor& desc,
        Octave const* octave);

    float keypoint_relative_scale (Keypoint const& kp);
    float keypoint_absolute_scale (Keypoint const& kp);

    void dump_octaves (void); // for debugging

private:
    mve::FloatImage::ConstPtr orig; // Original input image

    /* Octave parameters. */
    int min_octave; // Minimum octave, defaults to -1.
    int max_octave; // Maximum octave, defaults to 5.
    int octave_samples; // Samples in each octave, default = 3
    float pre_smoothing; // The amount of pre-smoothing, default = 1.6
    float inherent_blur; // The blur inherent in images, default = 0.5

    /* Keypoint filtering parameters. */
    float contrast_thres; // Feature contrast threshold
    float edge_ratio_thres; // Ratio of principal curvatures threshold

    /* Working data. */
    Octaves octaves; // The image pyramid (the octaves)
    Keypoints keypoints; // Detected keypoints
    Descriptors descriptors; // Final SIFT descriptors
};

/* ---------------------------------------------------------------- */

inline
Sift::Sift (void)
{
    this->min_octave = -1;
    this->max_octave = 4;
    this->octave_samples = 3;
    this->contrast_thres = 0.02f / (float)this->octave_samples;
    this->edge_ratio_thres = 10.0f;
    this->pre_smoothing = 1.6f;
    this->inherent_blur = 0.5f;
}

inline void
Sift::set_samples_per_octave (int samples)
{
    this->octave_samples = samples;
}

inline void
Sift::set_min_max_octave (int min_octave, int max_octave)
{
    this->min_octave = std::max(-1, min_octave);
    this->max_octave = max_octave;
}

inline void
Sift::set_contrast_threshold (float thres)
{
    this->contrast_thres = thres;
}

inline void
Sift::set_edge_threshold (float thres)
{
    this->edge_ratio_thres = thres;
}

inline void
Sift::set_inherent_blur (float sigma)
{
    this->inherent_blur = sigma;
}

inline void
Sift::set_pre_smoothing (float sigma)
{
    this->pre_smoothing = sigma;
}

inline Sift::Keypoints const&
Sift::get_keypoints (void) const
{
    return this->keypoints;
}

inline Sift::Descriptors const&
Sift::get_descriptors (void) const
{
    return this->descriptors;
}

SFM_NAMESPACE_END

#endif /* SFM_SIFT_HEADER */
