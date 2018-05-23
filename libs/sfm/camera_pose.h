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
#include "math/matrix_tools.h"
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
public:
    CameraPose (void);

    /** Initializes R with identity and t with zeros. */
    void init_canonical_form (void);
    /** Returns the P matrix as the product K [R | t]. */
    void fill_p_matrix (math::Matrix<double, 3, 4>* result) const;
    /** Initializes the K matrix from focal length and principal point. */
    void set_k_matrix (double flen, double px, double py);
    /** Returns the focal length as average of x and y focal length. */
    double get_focal_length (void) const;
    /** Returns the principal point (x factor). */
    double get_px (void) const;
    /** Returns the principal point (y factor). */
    double get_py (void) const;
    /** Returns the camera position (requires valid camera). */
    void fill_camera_pos (math::Vector<double, 3>* camera_pos) const;
    /** Returns true if K matrix is valid (non-zero focal length). */
    bool is_valid (void) const;

public:
    math::Matrix<double, 3, 3> K;
    math::Matrix<double, 3, 3> R;
    math::Vector<double, 3> t;
};

/* ------------------------ Implementation ------------------------ */

inline
CameraPose::CameraPose (void)
    : K(0.0)
    , R(0.0)
    , t(0.0)
{
}

inline void
CameraPose::init_canonical_form (void)
{
    math::matrix_set_identity(&this->R);
    this->t.fill(0.0);
}

inline void
CameraPose::fill_p_matrix (math::Matrix<double, 3, 4>* P) const
{
    math::Matrix<double, 3, 3> KR = this->K * this->R;
    math::Matrix<double, 3, 1> Kt(*(this->K * this->t));
    *P = KR.hstack(Kt);
}

inline void
CameraPose::set_k_matrix (double flen, double px, double py)
{
    this->K.fill(0.0);
    this->K[0] = flen; this->K[2] = px;
    this->K[4] = flen; this->K[5] = py;
    this->K[8] = 1.0;
}

inline double
CameraPose::get_focal_length (void) const
{
    return (this->K[0] + this->K[4]) / 2.0;
}

inline double
CameraPose::get_px (void) const
{
    return this->K[2];
}

inline double
CameraPose::get_py (void) const
{
    return this->K[5];
}

inline void
CameraPose::fill_camera_pos (math::Vector<double, 3>* camera_pos) const
{
    *camera_pos = -this->R.transposed().mult(this->t);
}

inline bool
CameraPose::is_valid (void) const
{
    return this->K[0] != 0.0;
}

SFM_NAMESPACE_END

#endif /* SFM_POSE_HEADER */
