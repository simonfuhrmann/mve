#include <iostream>
#include "math/matrix.h"

#include "pose.h"
#include "eigenwrapper.h"

int
main (int argc, char** argv)
{
    sfm::Eight2DPoints p1;
    sfm::Eight2DPoints p2;


    sfm::FundamentalMatrix f;
    //pose_8_point(p1, p2, &f);

    math::Matrix<double, 4, 2> mat_a;
    math::Matrix<double, 4, 4> mat_u;
    math::Matrix<double, 4, 2> mat_s;
    math::Matrix<double, 2, 2> mat_v;

    mat_a(0, 0) = 1; mat_a(0, 1) = 2;
    mat_a(1, 0) = 2; mat_a(1, 1) = 3;
    mat_a(2, 0) = 4; mat_a(2, 1) = 5;
    mat_a(3, 0) = 6; mat_a(3, 1) = 7;

    math::matrix_svd(mat_a, &mat_u, &mat_s, &mat_v);
    std::cout << "Matrix A:" << std::endl << mat_a << std::endl;
    std::cout << "Matrix U:" << std::endl << mat_u << std::endl;
    std::cout << "Matrix S:" << std::endl << mat_s << std::endl;
    std::cout << "Matrix V:" << std::endl << mat_v << std::endl;
    std::cout << "Result:" << std::endl << (mat_u * mat_s * mat_v.transposed()) << std::endl;

    return 0;
}

