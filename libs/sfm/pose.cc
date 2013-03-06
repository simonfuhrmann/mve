#include "math/matrixtools.h"

#include "pose.h"

SFM_NAMESPACE_BEGIN

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

SFM_NAMESPACE_END
