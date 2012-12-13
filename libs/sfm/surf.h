/*
 * SURF implementation.
 * Written by Simon Fuhrmann.
 *
 * This collection of classes implement the SURF keypoint detector and
 * descriptor as described in:
 *
 *   Speeded-Up Robust Features (SURF)
 *   by Herbert Bay, Andreas Ess, Tinne Tuytelaars, and Luc Van Gool
 *
 * Some useful references:
 * - "Resolving Implementation Ambiguity and Improving SURF" by Peter Abeles
 * - SURF Article at http://www.ipol.im/pub/pre/H2/
 *
 * Things still TODO:
 * - N9 Neighborhood is implemented twice. Re-use code.
 */
#ifndef MVE_SURFLIB_HEADER
#define MVE_SURFLIB_HEADER

#include <sys/types.h>
#include <vector>

#include "mve/image.h"

#include "defines.h"

SFM_NAMESPACE_BEGIN

/**
 * Representation of a SURF keypoint.
 */
struct SurfKeypoint
{
    int octave; ///< Octave index of the keypoint
    float sample; ///< Scale space sample index within octave in [0, 3]
    float x; ///< Detected keypoint X coordinate
    float y; ///< Detected keypoint X coordinate
};

/**
 * Representation of a SURF descriptor.
 */
struct SurfDescriptor
{
    float x;
    float y;
    float scale;
    float orientation;
    std::vector<float> data;
};

/*
 * Representation of a SURF octave.
 */
struct SurfOctave
{
    typedef float RespType; ///< Type for the Hessian response value
    typedef mve::Image<RespType> RespImage; ///< Hessian response map type
    typedef std::vector<RespImage::Ptr> RespImages; ///< Vector of response images
    RespImages imgs;
};

/**
 * Unfinished SURF implementation.
 *
 * Since SURF relies on summed area tables (SAT), it can currently only
 * be used with integer images, in particular byte images. For floating
 * point data types, SATs may be inaccurate due to possible large values
 * in the lower-right region of the SAT.
 */
class Surf
{
public:
    typedef int64_t SatType; ///< Signed type for the SAT image values
    typedef mve::Image<SatType> SatImage; ///< SAT image type
    typedef std::vector<SurfKeypoint> SurfKeypoints;
    typedef std::vector<SurfDescriptor> SurfDescriptors;
    typedef std::vector<SurfOctave> SurfOctaves;

public:
    Surf (void);

    /** Sets the input image. */
    void set_image (mve::ByteImage::ConstPtr image);
    /** Sets the hessian threshold, defaults to 10.0. */
    void set_contrast_threshold (float thres);
    /** Sets whether to generate the upright descriptor version. */
    void set_upright_descriptor (bool upright);

    /** Starts Surf keypoint detection and descriptor extraction. */
    void process (void);

    /** Returns the list of keypoints. */
    SurfKeypoints const& get_keypoints (void) const;
    /** Returns the list of descriptors. */
    SurfDescriptors const& get_descriptors (void) const;

    void draw_features (mve::ByteImage::Ptr image);

protected:
    void create_octaves (void);

    void create_response_map (int o, int k);
    SatType filter_dxx (int fs, int x, int y);
    SatType filter_dyy (int fs, int x, int y);
    SatType filter_dxy (int fs, int x, int y);

    void extrema_detection (void);
    void check_maximum (int o, int s, int x, int y);

    void keypoint_localization_and_filtering (void);
    bool keypoint_localization (SurfKeypoint* kp);

    void descriptor_assignment (void);
    bool descriptor_orientation (SurfDescriptor* descr);
    bool descriptor_computation (SurfDescriptor* descr, bool upright);
    void filter_dx_dy(int x, int y, int fs, float* dx, float* dy);

private:
    float contrast_thres;
    bool upright_descriptor;

    SatImage::Ptr sat;
    SurfOctaves octaves;
    SurfKeypoints keypoints;
    SurfDescriptors descriptors;
};

/* ---------------------------------------------------------------- */

inline
Surf::Surf (void)
    : contrast_thres(10.0f)
    , upright_descriptor(false)
{
}

inline void
Surf::set_contrast_threshold (float thres)
{
    this->contrast_thres = thres;
}

inline void
Surf::set_upright_descriptor (bool upright)
{
    this->upright_descriptor = upright;
}

inline Surf::SurfKeypoints const&
Surf::get_keypoints (void) const
{
    return this->keypoints;
}

inline Surf::SurfDescriptors const&
Surf::get_descriptors (void) const
{
    return this->descriptors;
}

SFM_NAMESPACE_END

#endif /* MVE_SURFLIB_HEADER */
