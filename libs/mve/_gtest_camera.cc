// Test cases for the MVE camera class.
// Written by Simon Fuhrmann.

#include <gtest/gtest.h>

#include "math/matrix.h"
#include "mve/camera.h"

TEST(MveCameraTest, CalibrationAndInverseTest)
{
    /* Test if K * K^-1 = I. */
    mve::CameraInfo cam;
    cam.flen = 1.0f;
    math::Matrix3f K, Kinv;
    cam.fill_calibration(*K, 800, 600);
    cam.fill_inverse_calibration(*Kinv, 800, 600);
    math::Matrix3f I = K * Kinv;
    for (int i = 0; i < 9; ++i)
        if (i == 0 || i == 4 || i == 8)
            EXPECT_NEAR(1.0f, I[i], 1e-10f);
        else
            EXPECT_NEAR(0.0f, I[i], 1e-10f);

    I = Kinv * K;
    for (int i = 0; i < 9; ++i)
        if (i == 0 || i == 4 || i == 8)
            EXPECT_NEAR(1.0f, I[i], 1e-10f);
        else
            EXPECT_NEAR(0.0f, I[i], 1e-10f);
}
