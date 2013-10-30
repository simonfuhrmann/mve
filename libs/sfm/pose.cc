#include "math/matrix_tools.h"
#include "sfm/pose.h"
#include "sfm/matrixsvd.h"
#include "sfm/matrixqrdecomp.h"

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

void
pose_from_2d_3d_correspondences (Correspondences2D3D const& corresp,
    math::Matrix<double, 3, 4>* p_matrix)
{
    std::vector<double> A(corresp.size() * 2 * 12);
    for (std::size_t i = 0; i < corresp.size(); ++i)
    {
        std::size_t const row_off_1 = (i * 2 + 0) * 12;
        std::size_t const row_off_2 = (i * 2 + 1) * 12;
        Correspondence2D3D const& c = corresp[i];
        A[row_off_1 +  4] = -c.p3d[0];
        A[row_off_1 +  5] = -c.p3d[1];
        A[row_off_1 +  6] = -c.p3d[2];
        A[row_off_1 +  7] = -1.0;
        A[row_off_1 +  8] = c.p2d[1] * c.p3d[0];
        A[row_off_1 +  9] = c.p2d[1] * c.p3d[1];
        A[row_off_1 + 10] = c.p2d[1] * c.p3d[2];
        A[row_off_1 + 11] = c.p2d[1] * 1.0;

        A[row_off_2 +  0] = c.p3d[0];
        A[row_off_2 +  1] = c.p3d[1];
        A[row_off_2 +  2] = c.p3d[2];
        A[row_off_2 +  3] = 1.0;
        A[row_off_2 +  8] = -c.p2d[0] * c.p3d[0];
        A[row_off_2 +  9] = -c.p2d[0] * c.p3d[1];
        A[row_off_2 + 10] = -c.p2d[0] * c.p3d[2];
        A[row_off_2 + 11] = -c.p2d[0] * 1.0;
    }

    std::vector<double> V(12 * 12);
    math::matrix_svd<double>(&A[0], corresp.size() * 2, 12, NULL, NULL, &V[0]);
    for (int i = 0; i < 12; ++i)
        (*p_matrix)[i] = V[i * 12 + 11];
}

void
pose_from_p_matrix (math::Matrix<double, 3, 4> const& p_matrix,
    CameraPose* pose)
{
    /* Take the 3x3 left sub-matrix of P, ignoring translation for now. */
    math::Matrix<double, 3, 3> p_sub;
    std::copy(*p_matrix + 0, *p_matrix + 0 + 3, *p_sub);
    std::copy(*p_matrix + 4, *p_matrix + 4 + 3, *p_sub + 3);
    std::copy(*p_matrix + 8, *p_matrix + 8 + 3, *p_sub + 6);

    /*
     * Perform RQ decomposition of submatrix of P. This is done using QR and
     * applying a permutation matrix P such that the following holds:
     *   A = QR  <=>  PA = PR PQ
     * The permutation performed is a combination of transpose and rotate.
     */
    p_sub.transpose();
    math::matrix_rotate_180_inplace(&p_sub);
    math::Matrix<double, 3, 3> Q, R;
    math::matrix_qr_decomp(p_sub, &Q, &R);
    R.transpose();
    Q.transpose();
    math::matrix_rotate_180_inplace(&R);
    math::matrix_rotate_180_inplace(&Q);

    /*
     * To obtain a proper calibration matrix, make sure R's diagonal entries
     * are positive. A negative entry is corrected by negating R's column
     * and Q's corresponding row.
     */
    for (int i = 0; i < 2; ++i)
        if (R(i, i) / R[8] < 0.0)
            for (int k = 0; k < 3; ++k)
            {
                R(k, i) = -R(k, i);
                Q(i, k) = -Q(i, k);
            }

    /* Translation t is K^-1 multiplied with the last column of P. */
    math::Vector<double, 3> trans = math::matrix_inverse(R) * p_matrix.col(3);

    /* Q of P = RQ is the rotation R of P = K [R|t]. */
    if (math::matrix_determinant(Q) < 0.0)
    {
        pose->R = -Q;
        pose->t = -trans;
    }
    else
    {
        pose->R = Q;
        pose->t = trans;
    }

    /* The K matrix is rescaled such that the lower right entry becomes 1. */
    for (int i = 0; i < 9; ++i)
        pose->K[i] = R[i] / R[8];
}

SFM_NAMESPACE_END
