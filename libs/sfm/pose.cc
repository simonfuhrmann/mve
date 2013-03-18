#include "math/matrixtools.h"

#include "pose.h"
#include "matrixsvd.h"

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

#if 0
dlib::matrix<double, 3,4>
create_P_from_3d_2d(const std::vector<math::Vec3d>& pos_3d,
                    const std::vector<math::Vec2d>& pos_2d)
{
    dlib::matrix<double, 3,4> P;
    if (pos_3d.size() != pos_2d.size())
    {
        std::cout << "Error sizes are not equal!" << std::endl;
        return P;
    }

    std::size_t num_corresp = pos_3d.size();

    dlib::matrix<double> A = dlib::zeros_matrix<double>(num_corresp*2,12);

    std::vector<dlib::matrix<double,3,1> > imagep;
    std::vector<dlib::matrix<double,4,1> > scenep;
    dlib::matrix<double, 3, 3> Ti;
    dlib::matrix<double, 4, 4> Ts;

    bundletools::normalize_points_2d(pos_2d, imagep, Ti);
    bundletools::normalize_points_3d(pos_3d, scenep, Ts);

    for (std::size_t i = 0; i < num_corresp; ++i)
    {
        A(2*i, 4) = -scenep[i](0);
        A(2*i, 5) = -scenep[i](1);
        A(2*i, 6) = -scenep[i](2);
        A(2*i, 7) = -1.0f;
        A(2*i, 8) = imagep[i](1) * scenep[i](0);
        A(2*i, 9) = imagep[i](1) * scenep[i](1);
        A(2*i, 10)= imagep[i](1) * scenep[i](2);
        A(2*i, 11)= imagep[i](1);

        A(2*i+1, 0) = scenep[i](0);
        A(2*i+1, 1) = scenep[i](1);
        A(2*i+1, 2) = scenep[i](2);
        A(2*i+1, 3) = 1.0f;
        A(2*i+1, 8) = -imagep[i](0) * scenep[i](0);
        A(2*i+1, 9) = -imagep[i](0) * scenep[i](1);
        A(2*i+1, 10)= -imagep[i](0) * scenep[i](2);
        A(2*i+1, 11)= -imagep[i](0);
    }
    dlib::matrix<double> U,S,V;
    dlib::svd(A, U, S, V);
    sort_singular(S, U, V);

    int cid = V.nc()-1;
    P(0,0) = V(0,cid); P(0,1) = V(1,cid); P(0,2) = V( 2,cid); P(0,3) = V( 3,cid);
    P(1,0) = V(4,cid); P(1,1) = V(5,cid); P(1,2) = V( 6,cid); P(1,3) = V( 7,cid);
    P(2,0) = V(8,cid); P(2,1) = V(9,cid); P(2,2) = V(10,cid); P(2,3) = V(11,cid);

    P = dlib::inv(Ti)*P*Ts;

    return P;
}
#endif
SFM_NAMESPACE_END
