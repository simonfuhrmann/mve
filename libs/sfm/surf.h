/*
 * SURF implementation.
 * Written by Simon Fuhrmann.
 */
#ifndef MVE_SURFLIB_HEADER
#define MVE_SURFLIB_HEADER

#include <sys/types.h>
#include <vector>

#include "mve/image.h"

#include "defines.h"

SFM_NAMESPACE_BEGIN

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
    typedef std::vector<SurfOctave> SurfOctaves;

public:
    Surf (void);

    /** Sets the input image. */
    void set_image (mve::ByteImage::ConstPtr image);
    /** Sets the hessian threshold, defaults to 10.0. */
    void set_contrast_threshold (float thres);

    /** Starts Surf keypoint detection and descriptor extraction. */
    void process (void);

    /** Returns the list of keypoints. */
    SurfKeypoints const& get_keypoints (void) const;

private:
    void create_octaves (void);

    void create_response_map (int o, int k);
    SatType filter_dxx (int fs, int x, int y);
    SatType filter_dyy (int fs, int x, int y);
    SatType filter_dxy (int fs, int x, int y);

    void extrema_detection (void);
    void check_maximum (int o, int s, int x, int y);

    void keypoint_localization_and_filtering (void);
    bool keypoint_localization (SurfKeypoint* kp);

private:
    mve::ByteImage::ConstPtr orig;
    float contrast_thres;

    SatImage::Ptr sat;
    SurfOctaves octaves;
    SurfKeypoints keypoints;
};

/* ---------------------------------------------------------------- */

inline
Surf::Surf (void)
    : contrast_thres(10.0f)
{
}

inline void
Surf::set_image (mve::ByteImage::ConstPtr image)
{
    this->orig = image;
}

inline Surf::SurfKeypoints const&
Surf::get_keypoints (void) const
{
    return this->keypoints;
}

inline void
Surf::set_contrast_threshold (float thres)
{
    this->contrast_thres = thres;
}

SFM_NAMESPACE_END

#endif /* MVE_SURFLIB_HEADER */
