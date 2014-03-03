#include <limits>
#include <iostream>

#include "sfm/triangulate.h"
#include "sfm/bundler_incremental.h"

SFM_NAMESPACE_BEGIN
SFM_BUNDLER_NAMESPACE_BEGIN

mve::Bundle::Ptr
Incremental::compute (ViewportList const& viewports, TrackList* tracks)
{
    /*
     * Overall process:
     * - Reconstruct fundamental for initial pair
     * - Extract pose for initial pair
     * - Run bundle adjustment
     * - Repeat
     *   - Find next view for reconstruction
     *   - Recover next camera pose
     *   - Reconstruct new tracks
     *   - Run bundle adjustment
     *
     */


    /* Initialize cameras. */
    this->cameras.clear();
    this->cameras.resize(viewports.size());

    /* Set track positions to invalid state. */
    for (std::size_t i = 0; i < tracks->size(); ++i)
    {
        Track& track = tracks->at(i);
        std::fill(track.pos.begin(), track.pos.end(),
            std::numeric_limits<float>::quiet_NaN());
    }

    /* Reconstruct fundamental matrix for initial pair. */
    this->compute_pose_for_initial_pair(this->opts.initial_pair_view_1_id,
        this->opts.initial_pair_view_2_id, viewports, *tracks);

    /* Reconstruct track positions with the intial pair. */
    this->triangulate_new_tracks(viewports, tracks);

    /* Run bundle adjustment. */
    this->bundle_adjustment(viewports, tracks);

    /* Reconstruct remaining views. */
    while (true)
    {
        int next_view_id = this->find_next_view(*tracks);
        if (next_view_id < 0)
            break;
        this->add_next_view(next_view_id, viewports, tracks);
        this->triangulate_new_tracks(viewports, tracks);
    }

    /* Create bundle data structure. */
    mve::Bundle::Ptr bundle = mve::Bundle::create();
    {
        /* Populate the cameras in the bundle. */
        mve::Bundle::Cameras& bundle_cams = bundle->get_cameras();
        bundle_cams.resize(this->cameras.size());
        for (std::size_t i = 0; i < this->cameras.size(); ++i)
        {
            mve::CameraInfo& cam = bundle_cams[i];
            CameraPose const& pose = this->cameras[i];
            Viewport const& viewport = viewports[i];
            if (!pose.is_valid())
            {
                cam.flen = 0.0f;
                continue;
            }

            float width = static_cast<float>(viewport.width);
            float height = static_cast<float>(viewport.height);
            float maxdim = std::max(width, height);
            cam.flen = static_cast<float>((pose.K[0] + pose.K[4]) / 2.0);
            cam.flen /= maxdim;
            cam.ppoint[0] = static_cast<float>(pose.K[2]) / width;
            cam.ppoint[1] = static_cast<float>(pose.K[5]) / height;
            std::copy(pose.R.begin(), pose.R.end(), cam.rot);
            std::copy(pose.t.begin(), pose.t.end(), cam.trans);
        }

        /* Populate the features in the Bundle. */
        mve::Bundle::Features& bundle_feats = bundle->get_features();
        for (std::size_t i = 0; i < tracks->size(); ++i)
        {
            Track const& track = tracks->at(i);
            if (std::isnan(track.pos[0]))
                continue;

            /* Copy position and color of the track. */
            bundle_feats.push_back(mve::Bundle::Feature3D());
            mve::Bundle::Feature3D& f3d = bundle_feats.back();
            std::copy(track.pos.begin(), track.pos.end(), f3d.pos);
            f3d.color[0] = track.color[0] / 255.0f;
            f3d.color[1] = track.color[1] / 255.0f;
            f3d.color[2] = track.color[2] / 255.0f;
            for (std::size_t j = 0; j < track.features.size(); ++j)
            {
                /* For each reference copy view ID, feature ID and 2D pos. */
                f3d.refs.push_back(mve::Bundle::Feature2D());
                mve::Bundle::Feature2D& f2d = f3d.refs.back();
                f2d.view_id = track.features[j].view_id;
                f2d.feature_id = track.features[j].feature_id;

                Viewport view = viewports[f2d.view_id];
                math::Vec2f const& f2d_pos = view.positions[f2d.feature_id];
                std::copy(f2d_pos.begin(), f2d_pos.end(), f2d.pos);
            }
        }
    }

    return bundle;
}

/* ---------------------------------------------------------------- */

void
Incremental::compute_pose_for_initial_pair (int view_1_id, int view_2_id,
    ViewportList const& viewports, TrackList const& tracks)
{
    Viewport const& view_1 = viewports[view_1_id];
    Viewport const& view_2 = viewports[view_2_id];

    if (this->opts.verbose_output)
    {
        std::cout << "  Computing fundamental matrix for "
            << "initial pair..." << std::endl;
    }

    /* Find the set of fundamental inliers. */
    Correspondences inliers;
    {
        /* Interate all tracks and find correspondences between the pair. */
        Correspondences correspondences;
        for (std::size_t i = 0; i < tracks.size(); ++i)
        {
            int view_1_feature_id = -1;
            int view_2_feature_id = -1;
            Track const& track = tracks[i];
            for (std::size_t j = 0; j < track.features.size(); ++j)
            {
                if (track.features[j].view_id == view_1_id)
                    view_1_feature_id = track.features[j].feature_id;
                if (track.features[j].view_id == view_2_id)
                    view_2_feature_id = track.features[j].feature_id;
            }

            if (view_1_feature_id != -1 && view_2_feature_id != -1)
            {
                math::Vec2f const& pos1 = view_1.positions[view_1_feature_id];
                math::Vec2f const& pos2 = view_2.positions[view_2_feature_id];
                Correspondence correspondence;
                std::copy(pos1.begin(), pos1.end(), correspondence.p1);
                std::copy(pos2.begin(), pos2.end(), correspondence.p2);
                correspondences.push_back(correspondence);
            }
        }

        /* Use correspondences and compute fundamental matrix using RANSAC. */
        this->opts.fundamental_opts.already_normalized = false;
        this->opts.fundamental_opts.threshold = 3.0f;
        RansacFundamental::Result ransac_result;
        RansacFundamental fundamental_ransac(this->opts.fundamental_opts);
        fundamental_ransac.estimate(correspondences, &ransac_result);

        if (this->opts.verbose_output)
        {
            float num_matches = correspondences.size();
            float num_inliers = ransac_result.inliers.size();
            float percentage = num_inliers / num_matches;

            std::cout << "  Pair "
                << "(" << view_1_id << "," << view_2_id << "): "
                << num_matches << " tracks, "
                << num_inliers << " fundamental inliers ("
                << util::string::get_fixed(100.0f * percentage, 2)
                << "%)." << std::endl;
        }

        /* Build correspondences from inliers only. */
        std::size_t const num_inliers = ransac_result.inliers.size();
        inliers.resize(num_inliers);
        for (std::size_t i = 0; i < num_inliers; ++i)
            inliers[i] = correspondences[ransac_result.inliers[i]];
    }

    /* Save a test match for later to resolte camera pose ambiguity. */
    Correspondence test_match = inliers[0];

    /* Normalize inliers and re-compute fundamental. */
    FundamentalMatrix fundamental;
    {
        math::Matrix3d T1, T2;
        FundamentalMatrix F;
        compute_normalization(inliers, &T1, &T2);
        apply_normalization(T1, T2, &inliers);
        fundamental_least_squares(inliers, &F);
        enforce_fundamental_constraints(&F);
        fundamental = T2.transposed() * F * T1;
        inliers.clear();
    }

    if (this->opts.verbose_output)
    {
        std::cout << "  Extracting pose for initial pair..." << std::endl;
    }

    /* Compute pose from fundamental matrix. */
    CameraPose pose1, pose2;
    {
        /* Populate K-matrices. */
        int const width1 = view_1.width;
        int const height1 = view_1.height;
        int const width2 = view_2.width;
        int const height2 = view_2.height;
        double flen1 = view_1.focal_length
            * static_cast<double>(std::max(width1, height1));
        double flen2 = view_2.focal_length
            * static_cast<double>(std::max(width2, height2));
        pose1.set_k_matrix(flen1, width1 / 2.0, height1 / 2.0);
        pose1.init_canonical_form();
        pose2.set_k_matrix(flen2, width2 / 2.0, height2 / 2.0);

        /* Compute essential matrix from fundamental matrix (HZ (9.12)). */
        EssentialMatrix E = pose2.K.transposed() * fundamental * pose1.K;

        /* Compute pose from essential. */
        std::vector<CameraPose> poses;
        pose_from_essential(E, &poses);

        /* Find the correct pose using point test (HZ Fig. 9.12). */
        bool found_pose = false;
        for (std::size_t i = 0; i < poses.size(); ++i)
        {
            poses[i].K = pose2.K;
            if (is_consistent_pose(test_match, pose1, poses[i]))
            {
                pose2 = poses[i];
                found_pose = true;
                break;
            }
        }

        if (!found_pose)
            throw std::runtime_error("Could not find valid initial pose");
    }

    /* Store recovered pose in viewport. */
    this->cameras[view_1_id] = pose1;
    this->cameras[view_2_id] = pose2;
}

/* ---------------------------------------------------------------- */

void
Incremental::triangulate_new_tracks (ViewportList const& viewports,
    TrackList* tracks)
{
    for (std::size_t i = 0; i < tracks->size(); ++i)
    {
        Track& track = tracks->at(i);

        /* Skip tracks that have already been reconstructed. */
        if (!std::isnan(track.pos[0]))
            continue;

        /* Collect up to two valid cameras and 2D feature positions. */
        std::vector<int> valid_cameras;
        std::vector<int> valid_features;
        valid_cameras.reserve(2);
        valid_features.reserve(2);
        for (std::size_t j = 0; j < track.features.size(); ++j)
        {
            int const view_id = track.features[j].view_id;
            if (!this->cameras[view_id].is_valid())
                continue;

            int const feature_id = track.features[j].feature_id;
            valid_cameras.push_back(view_id);
            valid_features.push_back(feature_id);
            if (valid_cameras.size() == 2)
                break;
        }

        /* Skip tracks with too few valid cameras. */
        if (valid_cameras.size() < 2)
            continue;

        /* Triangulate track using valid cameras. */
        Correspondence match;
        {
            Viewport const& view1 = viewports[valid_cameras[0]];
            Viewport const& view2 = viewports[valid_cameras[1]];
            math::Vec2f const& pos1 = view1.positions[valid_features[0]];
            math::Vec2f const& pos2 = view2.positions[valid_features[1]];
            std::copy(pos1.begin(), pos1.end(), match.p1);
            std::copy(pos2.begin(), pos2.end(), match.p2);
        }
        CameraPose const& pose1 = this->cameras[valid_cameras[0]];
        CameraPose const& pose2 = this->cameras[valid_cameras[1]];
        track.pos = triangulate_match(match, pose1, pose2);
    }
}

void
Incremental::bundle_adjustment (ViewportList const& viewports,
    TrackList* tracks)
{
    // TODO
}

int
Incremental::find_next_view (TrackList const& tracks)
{
    // TODO
    return -1;
}

void
Incremental::add_next_view (int next_view_id, ViewportList const& viewports,
    TrackList* tracks)
{
    // TODO
}

SFM_BUNDLER_NAMESPACE_END
SFM_NAMESPACE_END
