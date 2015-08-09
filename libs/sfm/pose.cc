/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include "math/matrix_tools.h"
#include "math/matrix_svd.h"
#include "math/matrix_qr.h"
#include "sfm/pose.h"

SFM_NAMESPACE_BEGIN

CameraPose::CameraPose (void)
{
    this->K.fill(0.0f);
    this->R.fill(0.0f);
    this->t.fill(0.0f);
}

void
CameraPose::init_canonical_form (void)
{
    math::matrix_set_identity(*this->R, 3);
    this->t.fill(0.0);
}

void
CameraPose::fill_p_matrix (math::Matrix<double, 3, 4>* P) const
{
    math::Matrix<double, 3, 3> KR = this->K * this->R;
    math::Matrix<double, 3, 1> Kt(*(this->K * this->t));
    *P = KR.hstack(Kt);
}

void
CameraPose::set_k_matrix (double flen, double px, double py)
{
    this->K.fill(0.0);
    this->K[0] = flen; this->K[2] = px;
    this->K[4] = flen; this->K[5] = py;
    this->K[8] = 1.0;
}

double
CameraPose::get_focal_length (void) const
{
    return (this->K[0] + this->K[4]) / 2.0;
}

void
CameraPose::fill_camera_pos (math::Vector<double, 3>* camera_pos) const
{
    *camera_pos = -this->R.transposed().mult(this->t);
}

bool
CameraPose::is_valid (void) const
{
    return this->K[0] != 0.0f;
}

SFM_NAMESPACE_END
