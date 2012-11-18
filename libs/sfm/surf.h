/*
 * SURF implementation.
 * Written by Simon Fuhrmann.
 */
#ifndef MVE_SURFLIB_HEADER
#define MVE_SURFLIB_HEADER

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
    int o; ///< Octave index of the keypoint
    int ix; ///< Initially detected keypoint X coordinate
    int iy; ///< Initially detected keypoint X coordinate
    int is; ///< Scale space sample index within octave in {0, 1}
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

private:
    mve::ByteImage::ConstPtr orig;
    SatImage::Ptr sat;
    SurfOctaves octaves;
    SurfKeypoints keypoints;

private:
    void create_octaves (void);
    void extrema_detection (void);
    void extrema_detection (SurfOctave::RespImage::Ptr* samples, int o, int s);
    void create_response_map (int o, int k);
    SatType filter_dxx (int fs, int x, int y);
    SatType filter_dyy (int fs, int x, int y);
    SatType filter_dxy (int fs, int x, int y);

public:
    Surf (void);

    /** Sets the input image. */
    void set_image (mve::ByteImage::ConstPtr image);

    /** Starts Surf keypoint detection and descriptor extraction. */
    void process (void);

    /** Returns the list of keypoints. */
    SurfKeypoints const& get_keypoints (void) const;
};

/* ---------------------------------------------------------------- */

inline
Surf::Surf (void)
    // Init
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

SFM_NAMESPACE_END

#endif /* MVE_SURFLIB_HEADER */
