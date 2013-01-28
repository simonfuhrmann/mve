// Test cases for pose estimation.
// Written by Simon Fuhrmann.

#include <gtest/gtest.h>

#include "sfm/pose.h"

TEST(PoseTest, PointNormalization1)
{
    // Normalization of 3 points.
    math::Matrix<float, 3, 3> set;
    set[0] = 5.0f; set[1] = 5.0f; set[2] = 1.0f;
    set[3] = -5.0f; set[4] = -1.0f; set[5] = 1.0f;
    set[6] = 0.0f; set[7] = 0.0f; set[8] = 1.0f;
    math::Matrix<float, 3, 3> trans;
    sfm::pose_find_normalization(set, &trans);
    EXPECT_NEAR(trans[0], 0.1f, 1e-6f);
    EXPECT_NEAR(trans[1], 0.0f, 1e-6f);
    EXPECT_NEAR(trans[2], 0.0f, 1e-6f);
    EXPECT_NEAR(trans[3], 0.0f, 1e-6f);
    EXPECT_NEAR(trans[4], 0.1f, 1e-6f);
    EXPECT_NEAR(trans[5], -(5.0f + -1.0f + 0.0f)/3.0f/10.0f, 1e-6f);
    EXPECT_NEAR(trans[6], 0.0f, 1e-6f);
    EXPECT_NEAR(trans[7], 0.0f, 1e-6f);
    EXPECT_NEAR(trans[8], 1.0f, 1e-6f);
}

TEST(PoseTest, PointNormalization2)
{
    // Normalization of 2 points.
    math::Matrix<double, 2, 3> set;
    set[0] = -4.0; set[1] = 8.0; set[2] = 1.0f;
    set[3] = -5.0; set[4] = 10.0f; set[5] = 1.0f;
    math::Matrix<double, 3, 3> trans;
    sfm::pose_find_normalization(set, &trans);
    EXPECT_NEAR(trans[0], 0.5, 1e-6);
    EXPECT_NEAR(trans[1], 0.0, 1e-6);
    EXPECT_NEAR(trans[2], 4.5/2.0, 1e-6);
    EXPECT_NEAR(trans[3], 0.0, 1e-6);
    EXPECT_NEAR(trans[4], 0.5, 1e-6);
    EXPECT_NEAR(trans[5], -9.0/2.0, 1e-6);
    EXPECT_NEAR(trans[6], 0.0, 1e-6);
    EXPECT_NEAR(trans[7], 0.0, 1e-6);
    EXPECT_NEAR(trans[8], 1.0, 1e-6);
}

TEST(PostTest, Test8Point)
{
    sfm::Eight2DPoints p1;
    p1(0, 0) = 45;  p1(0, 1) = 210; p1(0, 2) = 1;
    p1(1, 0) = 253; p1(1, 1) = 211; p1(1, 2) = 1;
    p1(2, 0) = 154; p1(2, 1) = 188; p1(2, 2) = 1;
    p1(3, 0) = 27;  p1(3, 1) = 37;  p1(3, 2) = 1;
    p1(4, 0) = 209; p1(4, 1) = 164; p1(4, 2) = 1;
    p1(5, 0) = 33;  p1(5, 1) = 77;  p1(5, 2) = 1;
    p1(6, 0) = 93;  p1(6, 1) = 58;  p1(6, 2) = 1;
    p1(7, 0) = 66;  p1(7, 1) = 75;  p1(7, 2) = 1;

    sfm::Eight2DPoints p2;
    p2(0, 0) = 87;  p2(0, 1) = 216; p2(0, 2) = 1;
    p2(1, 0) = 285; p2(1, 1) = 216; p2(1, 2) = 1;
    p2(2, 0) = 188; p2(2, 1) = 194; p2(2, 2) = 1;
    p2(3, 0) = 51;  p2(3, 1) = 49;  p2(3, 2) = 1;
    p2(4, 0) = 234; p2(4, 1) = 171; p2(4, 2) = 1;
    p2(5, 0) = 56;  p2(5, 1) = 88;  p2(5, 2) = 1;
    p2(6, 0) = 114; p2(6, 1) = 69;  p2(6, 2) = 1;
    p2(7, 0) = 87;  p2(7, 1) = 86;  p2(7, 2) = 1;

    sfm::FundamentalMatrix F(0.0);
    sfm::pose_8_point(p1, p2, &F);

    sfm::FundamentalMatrix F2;
    F2(0,0) = 0.000000014805557;  F2(0,1) = 0.000002197550186;  F2(0,2) = 0.001632934316777;
    F2(1,0) = -0.000002283909471; F2(1,1) = -0.000001354336179; F2(1,2) = 0.008734421917905;
    F2(2,0) = -0.001472308151103; F2(2,1) = -0.008375559378962; F2(2,2) = -0.160734037191207;

    // Force Fundamental matrices to the same scale.
    F /= F(2,2);
    F2 /= F2(2,2);

    //std::cout << "Fundamental:" << std::endl << F << std::endl;
    //std::cout << "Fundamental 2: " << std::endl << F2 << std::endl;
    for (int i = 0; i < 9; ++i)
        EXPECT_NEAR((F[i] - F2[i]) / (F[i] + F2[i]), 0.0, 0.05);
}
