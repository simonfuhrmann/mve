/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 *
 * Some useful references:
 * - "Resolving Implementation Ambiguity and Improving SURF" by Peter Abeles
 * - SURF Article at http://www.ipol.im/pub/pre/H2/
 *
 * TODO:
 * - N9 Neighborhood is implemented twice. Re-use code.
 */
#ifndef SFM_SURF_HEADER
#define SFM_SURF_HEADER

#include <sys/types.h>
#include <vector>

#include "math/vector.h"
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
     * SURF options.
     */
    struct Options
    {
        Options (void);

        /** Sets the hessian threshold, defaults to 500.0. */
        float contrast_threshold;

        /** Trade rotation invariance for speed. Defaults to false. */
        bool use_upright_descriptor;

        /**
         * Produce status messages on the console.
         */
        bool verbose_output;

        /**
         * Produce even more messages on the console.
         */
        bool debug_output;
    };

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
     * The descriptor is created in a rotation invariant way. The resulting
     * vector is signed and normalized, and has 64 dimensions.
     */
    struct Descriptor
    {
        /** The sub-pixel x-coordinate of the image keypoint. */
        float x;
        /** The sub-pixel y-coordinate of the image keypoint. */
        float y;
        /** The scale (or sigma value) of the keypoint. */
        float scale;
        /** The orientation of the image keypoint in [-PI, PI]. */
        float orientation;
        /** The descriptor data, elements are signed in [-1.0, 1.0]. */
        math::Vector<float, 64> data;
    };

public:
    typedef std::vector<Keypoint> Keypoints;
    typedef std::vector<Descriptor> Descriptors;

public:
    explicit Surf (Options const& options);

    /** Sets the input image. */
    void set_image (mve::ByteImage::ConstPtr image);

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
    Options options;
    SatImage::Ptr sat;
    Octaves octaves;
    Keypoints keypoints;
    Descriptors descriptors;
};

/* ---------------------------------------------------------------- */

inline
Surf::Options::Options (void)
    : contrast_threshold(500.0f)
    , use_upright_descriptor(false)
    , verbose_output(false)
    , debug_output(false)
{
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

#endif /* SFM_SURF_HEADER */
