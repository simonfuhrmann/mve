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

void
CameraPose::set_from_p_and_known_k (math::Matrix<double, 3, 4> const&  p_matrix)
{
    /* There are several ways to obtain [R t] from P with known K. */

#if 0
    /*
     * This method computes K^-1 * P to get [R' t'] with non-orthogonal R'
     * and inaccurate t'. R' can then be constrained to an optimal rotation R,
     * and t = R * R'^-1 * t'. (FIXME is this updated t correct?)
     */
    math::Matrix3d K_inv = math::matrix_inverse(this->K);
    math::Matrix<double, 3, 4> Rt_bad = K_inv * p_matrix;
    math::Matrix<double, 3, 3> R_bad = Rt_bad.delete_col(3);
    this->R = matrix_optimal_rotation(R_bad);
    this->t = this->R * math::matrix_inverse(R_bad) * Rt_bad.col(3);
#endif

#if 0
    /*
     * This method computes QR decomposition of P and obtains K' [R t].
     * K' is then replaced with known K. This introduces inaccuracies when
     * computing the final P = K [R t].
     */
    CameraPose pose;
    pose_from_p_matrix(p_matrix, &pose);
    this->R = pose.R;
    this->t = pose.t;
#endif

#if 1
    /*
     * This method computes QR decomposition of P and obtains K' [R t].
     * Instead of using known K, K' is constrained to a valid K matrix.
     * The advantage is that this method does not need known focal length.
     */
    CameraPose pose;
    pose_from_p_matrix(p_matrix, &pose);
    this->R = pose.R;
    this->t = pose.t;

    pose.K /= pose.K[8];
    pose.K[2] = this->K[2];
    pose.K[5] = this->K[5];
    this->K = pose.K;
    this->K[0] = this->K[4] = (this->K[0] + this->K[4]) / 2.0;
    this->K[1] = this->K[3] = this->K[6] = this->K[7] = 0.0;
#endif

#if 0
    /*
     * This method computes the camera center c as the right zero-vector
     * of the projection P * c = 0. The rotation is recovered using known
     * calibration R' = K^-1 * M where M is the left 3x3 matrix of P.
     * The translation is then computed as t = -R * c.
     */
    math::Vector<double, 3> c;
    c[0] = math::matrix_determinant(p_matrix.delete_col(0));
    c[1] = -math::matrix_determinant(p_matrix.delete_col(1));
    c[2] = math::matrix_determinant(p_matrix.delete_col(2));
    c /= -math::matrix_determinant(p_matrix.delete_col(3));

    math::Matrix3d K_inv = math::matrix_inverse(this->K);
    math::Matrix<double, 3, 4> Rt_bad = K_inv * p_matrix;
    math::Matrix<double, 3, 3> R_bad = Rt_bad.delete_col(3);
    this->R = matrix_optimal_rotation(R_bad);
    this->t = -(this->R * c);
#endif

#if 0
    std::cout << "K-Matrix: " << std::endl;
    std::cout << this->K << std::endl;
    std::cout << "R-Matrix: " << std::endl;
    std::cout << this->R << std::endl;
    std::cout << "t-Vector: " << this->t << std::endl;
#endif
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

/* ---------------------------------------------------------------- */

void
pose_from_2d_3d_correspondences (Correspondences2D3D const& corresp,
    math::Matrix<double, 3, 4>* p_matrix)
{
    if (corresp.size() < 6)
        throw std::invalid_argument("At least 6 correspondences required");

    std::vector<double> A(corresp.size() * 2 * 12, 0.0f);
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

    /* Consider the last column of V as the solution. */
    for (int i = 0; i < 12; ++i)
        (*p_matrix)[i] = V[i * 12 + 11];
}

void
pose_from_p_matrix (math::Matrix<double, 3, 4> const& p_matrix,
    CameraPose* pose)
{
    /* Take the left 3x3 sub-matrix of P, ignoring translation for now. */
    math::Matrix<double, 3, 3> p_sub;
    std::copy(*p_matrix + 0, *p_matrix + 0 + 3, *p_sub);
    std::copy(*p_matrix + 4, *p_matrix + 4 + 3, *p_sub + 3);
    std::copy(*p_matrix + 8, *p_matrix + 8 + 3, *p_sub + 6);

    /*
     * Perform RQ decomposition of the upper left 3x3 submatrix of P.
     * This is done using QR and applying a permutation matrix X:
     *   A = QR  <=>  XA = XR XQ
     * The permutation performed is a combination of transpose and rotate.
     */
    p_sub.transpose();
    math::matrix_rotate_180_inplace(&p_sub);
    math::Matrix<double, 3, 3> Q, R;
    math::matrix_qr(p_sub, &Q, &R);
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

math::Matrix<double, 3, 3>
matrix_optimal_rotation (math::Matrix<double, 3, 3> const& matrix)
{
    /* Compute SVD of matrix A. */
    math::Matrix<double, 3, 3> mat_u, mat_v;
    math::matrix_svd<double, 3, 3>(matrix, &mat_u, NULL, &mat_v);

    /* Invert last column of V if input has negative determinant. */
    if (math::matrix_determinant(matrix) < 0.0)
    {
        mat_v[2] = -mat_v[2];
        mat_v[5] = -mat_v[5];
        mat_v[8] = -mat_v[8];
    }

    /* Return U C V^T where C is baked into V. */
    return mat_u.mult(mat_v.transposed());
}

SFM_NAMESPACE_END
