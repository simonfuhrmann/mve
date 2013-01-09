/*
 * SURF implementation.
 * Written by Simon Fuhrmann.
 *
 * Some useful references:
 * - "Resolving Implementation Ambiguity and Improving SURF" by Peter Abeles
 * - SURF Article at http://www.ipol.im/pub/pre/H2/
 *
 * TODO:
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
 * Implementation of the SURF feature detector and descriptor as described in:
 *
 *   Speeded-Up Robust Features (SURF)
 *   by Herbert Bay, Andreas Ess, Tinne Tuytelaars, and Luc Van Gool
 *
 * Since SURF relies on summed area tables (SAT), it can currently only
 * be used with integer images, in particular byte images. For floating
 * point data types, SATs may be inaccurate due to possible large values
 * in the lower-right region of the SAT.
 */
class Surf
{
public:
    /**
     * Representation of a SURF keypoint.
     */
    struct Keypoint
    {
        int octave; ///< Octave index of the keypoint
        float sample; ///< Scale space sample index within octave in [0, 3]
        float x; ///< Detected keypoint X coordinate
        float y; ///< Detected keypoint Y coordinate
    };

    /**
     * Representation of a SURF descriptor.
     */
    struct Descriptor
    {
        float x;
        float y;
        float scale;
        float orientation;
        std::vector<float> data;
    };

public:
    typedef std::vector<Keypoint> Keypoints;
    typedef std::vector<Descriptor> Descriptors;

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
    Keypoints const& get_keypoints (void) const;
    /** Returns the list of descriptors. */
    Descriptors const& get_descriptors (void) const;

protected:
    /*
     * Representation of a SURF octave.
     */
    struct Octave
    {
        typedef float RespType; ///< Type for the Hessian response value
        typedef mve::Image<RespType> RespImage; ///< Hessian response map type
        typedef std::vector<RespImage::Ptr> RespImages; ///< Vector of response images
        RespImages imgs;
    };

protected:
    typedef int64_t SatType; ///< Signed type for the SAT image values
    typedef mve::Image<SatType> SatImage; ///< SAT image type
    typedef std::vector<Octave> Octaves;

protected:
    void create_octaves (void);

    void create_response_map (int o, int k);
    SatType filter_dxx (int fs, int x, int y);
    SatType filter_dyy (int fs, int x, int y);
    SatType filter_dxy (int fs, int x, int y);

    void extrema_detection (void);
    void check_maximum (int o, int s, int x, int y);

    void keypoint_localization_and_filtering (void);
    bool keypoint_localization (Surf::Keypoint* kp);

    void descriptor_assignment (void);
    bool descriptor_orientation (Descriptor* descr);
    bool descriptor_computation (Descriptor* descr, bool upright);
    void filter_dx_dy(int x, int y, int fs, float* dx, float* dy);

private:
    float contrast_thres;
    bool upright_descriptor;

    SatImage::Ptr sat;
    Octaves octaves;
    Keypoints keypoints;
    Descriptors descriptors;
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

inline Surf::Keypoints const&
Surf::get_keypoints (void) const
{
    return this->keypoints;
}

inline Surf::Descriptors const&
Surf::get_descriptors (void) const
{
    return this->descriptors;
}

SFM_NAMESPACE_END

#endif /* MVE_SURFLIB_HEADER */
