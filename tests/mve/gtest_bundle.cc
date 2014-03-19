// Test cases for the MVE bundle class.
// Written by Simon Fuhrmann.

#include <gtest/gtest.h>

#include "mve/bundle.h"

TEST(BundleTest, BundleRemoveCamera)
{
    mve::Bundle::Ptr bundle = mve::Bundle::create();

    mve::Bundle::Cameras& cams = bundle->get_cameras();
    {
        mve::CameraInfo cam1, cam2;
        cam1.flen = 1.0f;
        cam2.flen = 2.0f;
        cams.push_back(cam1);
        cams.push_back(cam2);
    }

    mve::Bundle::Features& features = bundle->get_features();
    {
        mve::Bundle::Feature2D f2d_1;
        f2d_1.view_id = 0;
        f2d_1.feature_id = 0;
        mve::Bundle::Feature2D f2d_2;
        f2d_2.view_id = 1;
        f2d_2.feature_id = 0;

        mve::Bundle::Feature3D f3d_1;
        f3d_1.refs.push_back(f2d_1);
        f3d_1.refs.push_back(f2d_2);
        mve::Bundle::Feature3D f3d_2;
        f3d_2.refs.push_back(f2d_1);
        mve::Bundle::Feature3D f3d_3;
        f3d_3.refs.push_back(f2d_2);

        features.push_back(f3d_1);
        features.push_back(f3d_2);
        features.push_back(f3d_3);
    }

    // Delete a camera.
    bundle->delete_camera(1);

    // Check consistent bundle.
    ASSERT_EQ(2, cams.size());
    EXPECT_EQ(1.0f, cams[0].flen);
    EXPECT_EQ(0.0f, cams[1].flen);
    ASSERT_EQ(3, features.size());
    ASSERT_EQ(1, features[0].refs.size());
    ASSERT_EQ(1, features[1].refs.size());
    ASSERT_EQ(0, features[2].refs.size());
    ASSERT_EQ(0, features[0].refs[0].view_id);
    ASSERT_EQ(0, features[1].refs[0].view_id);
}
