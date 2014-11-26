// Test cases for the track generation component.
// Written by Simon Fuhrmann.

#include <gtest/gtest.h>

#include "sfm/bundler_tracks.h"

TEST(BundlerTracksTest, RemoveConflictsTest)
{
    sfm::bundler::ViewportList viewports;
    {
        sfm::bundler::Viewport v1;
        sfm::bundler::Viewport v2;
        sfm::bundler::Viewport v3;
        v1.features.colors.resize(8, math::Vec3uc(0, 0, 0));
        v1.features.positions.resize(8, math::Vec2f(0.0f));
        v2.features.colors.resize(9, math::Vec3uc(0, 0, 0));
        v2.features.positions.resize(9, math::Vec2f(0.0f));
        v3.features.colors.resize(10, math::Vec3uc(0, 0, 0));
        v3.features.positions.resize(10, math::Vec2f(0.0f));
        viewports.push_back(v1);
        viewports.push_back(v2);
        viewports.push_back(v3);
    }

    sfm::bundler::TwoViewMatching m10;
    m10.view_1_id = 1;
    m10.view_2_id = 0;
    m10.matches.push_back(sfm::CorrespondenceIndex(1, 0));
    m10.matches.push_back(sfm::CorrespondenceIndex(2, 2));
    m10.matches.push_back(sfm::CorrespondenceIndex(5, 5));
    m10.matches.push_back(sfm::CorrespondenceIndex(6, 5));
    m10.matches.push_back(sfm::CorrespondenceIndex(7, 7));

    sfm::bundler::TwoViewMatching m20;
    m20.view_1_id = 2;
    m20.view_2_id = 0;
    m20.matches.push_back(sfm::CorrespondenceIndex(2, 4));
    m20.matches.push_back(sfm::CorrespondenceIndex(8, 7));

    sfm::bundler::TwoViewMatching m21;
    m21.view_1_id = 2;
    m21.view_2_id = 1;
    m21.matches.push_back(sfm::CorrespondenceIndex(0, 1));
    m21.matches.push_back(sfm::CorrespondenceIndex(2, 2));
    m21.matches.push_back(sfm::CorrespondenceIndex(3, 4));
    m21.matches.push_back(sfm::CorrespondenceIndex(5, 5));
    m21.matches.push_back(sfm::CorrespondenceIndex(5, 6));
    m21.matches.push_back(sfm::CorrespondenceIndex(8, 7));

    sfm::bundler::PairwiseMatching matching;
    matching.push_back(m10);
    matching.push_back(m21);
    matching.push_back(m20);

    sfm::bundler::Tracks::Options options;
    options.verbose_output = true;
    sfm::bundler::TrackList track_list;
    sfm::bundler::Tracks tracks(options);
    tracks.compute(matching, &viewports, &track_list);

    // Check tracks (at least list sizes).
    ASSERT_EQ(3, track_list.size());
    ASSERT_EQ(3, track_list[0].features.size());
    ASSERT_EQ(3, track_list[1].features.size());
    ASSERT_EQ(2, track_list[2].features.size());
    // Check viewports and to-track mapping.
    ASSERT_EQ(8, viewports[0].track_ids.size());
    ASSERT_EQ(9, viewports[1].track_ids.size());
    ASSERT_EQ(10, viewports[2].track_ids.size());

    int track_ids_v0[] = { 0, -1, -1, -1, -1, -1, -1, 1 };
    int track_ids_v1[] = { -1, 0, -1, -1, 2, -1, -1, 1, -1 };
    int track_ids_v2[] = { 0, -1, -1, 2, -1, -1, -1, -1, 1, -1 };
    for (int i = 0; i < 8; ++i)
        EXPECT_EQ(track_ids_v0[i], viewports[0].track_ids[i]) << " v0:" << i;
    for (int i = 0; i < 9; ++i)
        EXPECT_EQ(track_ids_v1[i], viewports[1].track_ids[i]) << " v1:" << i;
    for (int i = 0; i < 10; ++i)
        EXPECT_EQ(track_ids_v2[i], viewports[2].track_ids[i]) << " v2:" << i;
}
