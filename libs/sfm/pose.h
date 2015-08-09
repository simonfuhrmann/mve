/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef SFM_POSE_HEADER
#define SFM_POSE_HEADER

#include <vector>

#include "math/vector.h"
#include "math/matrix.h"
#include "sfm/defines.h"
#include "sfm/correspondence.h"

SFM_NAMESPACE_BEGIN

/**
 * The camera pose is the 3x4 matrix P = K [R | t]. K is the 3x3 calibration
 * matrix, R a 3x3 rotation matrix and t a 3x1 translation vector.
 *
 *       | f  0  px |    The calibration matrix contains the focal length f,
 *   K = | 0  f  py |    and the principal point px and py.
 *       | 0  0   1 |
 *
 * For pose estimation, the calibration matrix is assumed to be known. This
 * might not be the case, but even a good guess of the focal length and the
 * principal point set to the image center can produce reasonably good
 * results so that bundle adjustment can recover better parameters.
 */
struct CameraPose
{
    CameraPose (void);

    math::Matrix<double, 3, 3> K;
    math::Matrix<double, 3, 3> R;
    math::Vector<double, 3> t;

    /** Initializes R with identity and t with zeros. */
    void init_canonical_form (void);
    /** Returns the P matrix as the product K [R | t]. */
    void fill_p_matrix (math::Matrix<double, 3, 4>* result) const;
    /** Initializes the K matrix from focal length and principal point. */
    void set_k_matrix (double flen, double px, double py);
    /** Returns the focal length as average of x and y focal length. */
    double get_focal_length (void) const;
    /** Returns the camera position (requires valid camera). */
    void fill_camera_pos (math::Vector<double, 3>* camera_pos) const;
    /** Returns true if K matrix is valid (non-zero focal length). */
    bool is_valid (void) const;
};

/** List of camera poses. */
typedef std::vector<CameraPose> CameraPoseList;

SFM_NAMESPACE_END

#endif /* SFM_POSE_HEADER */
