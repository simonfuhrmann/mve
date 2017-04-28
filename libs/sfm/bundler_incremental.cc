/*
 * Copyright (C) 2015, Simon Fuhrmann, Fabian Langguth
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <limits>
#include <iostream>
#include <utility>

#include "util/timer.h"
#include "math/transform.h"
#include "math/matrix_svd.h"
#include "math/matrix_tools.h"
#include "sfm/triangulate.h"
#include "sfm/bundle_adjustment.h"
#include "sfm/bundler_incremental.h"

SFM_NAMESPACE_BEGIN
SFM_BUNDLER_NAMESPACE_BEGIN

void
Incremental::initialize (ViewportList* viewports, TrackList* tracks,
    SurveyPointList* survey_points)
{
    this->viewports = viewports;
    this->tracks = tracks;
    this->survey_points = survey_points;

    if (this->viewports->empty())
        throw std::invalid_argument("No viewports given");

    /* Check if at least two cameras are initialized. */
    std::size_t num_valid_cameras = 0;
    for (std::size_t i = 0; i < this->viewports->size(); ++i)
        if (this->viewports->at(i).pose.is_valid())
            num_valid_cameras += 1;
    if (num_valid_cameras < 2)
        throw std::invalid_argument("Two or more valid cameras required");

    /* Set track positions to invalid state. */
    for (std::size_t i = 0; i < tracks->size(); ++i)
    {
        Track& track = tracks->at(i);
        track.invalidate();
    }
}

/* ---------------------------------------------------------------- */

void
Incremental::find_next_views (std::vector<int>* next_views)
{
    /* Create mapping from valid tracks to view ID. */
    std::vector<std::pair<int, int> > valid_tracks(this->viewports->size());
    for (std::size_t i = 0; i < valid_tracks.size(); ++i)
        valid_tracks[i] = std::make_pair(0, static_cast<int>(i));

    for (std::size_t i = 0; i < this->tracks->size(); ++i)
    {
        Track const& track = this->tracks->at(i);
        if (!track.is_valid())
            continue;

        for (std::size_t j = 0; j < track.features.size(); ++j)
        {
            int const view_id = track.features[j].view_id;
            if (this->viewports->at(view_id).pose.is_valid())
                continue;
            valid_tracks[view_id].first += 1;
        }
    }

    /* Sort descending by number of valid tracks. */
    std::sort(valid_tracks.rbegin(), valid_tracks.rend());

    /* Return unreconstructed views with most valid tracks. */
    next_views->clear();
    for (std::size_t i = 0; i < valid_tracks.size(); ++i)
    {
        if (valid_tracks[i].first > 6)
            next_views->push_back(valid_tracks[i].second);
    }
}

/* ---------------------------------------------------------------- */

bool
Incremental::reconstruct_next_view (int view_id)
{
    Viewport const& viewport = this->viewports->at(view_id);
    FeatureSet const& features = viewport.features;

    /* Collect all 2D-3D correspondences. */
    Correspondences2D3D corr;
    std::vector<int> track_ids;
    std::vector<int> feature_ids;
    for (std::size_t i = 0; i < viewport.track_ids.size(); ++i)
    {
        int const track_id = viewport.track_ids[i];
        if (track_id < 0 || !this->tracks->at(track_id).is_valid())
            continue;
        math::Vec2f const& pos2d = features.positions[i];
        math::Vec3f const& pos3d = this->tracks->at(track_id).pos;

        corr.push_back(Correspondence2D3D());
        Correspondence2D3D& c = corr.back();
        std::copy(pos3d.begin(), pos3d.end(), c.p3d);
        std::copy(pos2d.begin(), pos2d.end(), c.p2d);
        track_ids.push_back(track_id);
        feature_ids.push_back(i);
    }

    if (this->opts.verbose_output)
    {
        std::cout << "Collected " << corr.size()
            << " 2D-3D correspondences." << std::endl;
    }

    /* Initialize a temporary camera. */
    CameraPose temp_camera;
    temp_camera.set_k_matrix(viewport.focal_length, 0.0, 0.0);

    /* Compute pose from 2D-3D correspondences using P3P. */
    util::WallTimer timer;
    RansacPoseP3P::Result ransac_result;
    {
        RansacPoseP3P ransac(this->opts.pose_p3p_opts);
        ransac.estimate(corr, temp_camera.K, &ransac_result);
    }

    /* Cancel if inliers are below a 33% threshold. */
    if (3 * ransac_result.inliers.size() < corr.size())
    {
        if (this->opts.verbose_output)
            std::cout << "Only " << ransac_result.inliers.size()
                << " 2D-3D correspondences inliers ("
                << (100 * ransac_result.inliers.size() / corr.size())
                << "%). Skipping view." << std::endl;
        return false;
    }
    else if (this->opts.verbose_output)
    {
        std::cout << "Selected " << ransac_result.inliers.size()
            << " 2D-3D correspondences inliers ("
            << (100 * ransac_result.inliers.size() / corr.size())
            << "%), took " << timer.get_elapsed() << "ms." << std::endl;
    }

    /*
     * Remove outliers from tracks and tracks from viewport.
     * TODO: Once single cam BA has been performed and parameters for this
     * camera are optimized, evaluate outlier tracks and try to restore them.
     */
    for (std::size_t i = 0; i < ransac_result.inliers.size(); ++i)
        track_ids[ransac_result.inliers[i]] = -1;
    for (std::size_t i = 0; i < track_ids.size(); ++i)
    {
        if (track_ids[i] < 0)
            continue;
        this->tracks->at(track_ids[i]).remove_view(view_id);
        this->viewports->at(view_id).track_ids[feature_ids[i]] = -1;
    }
    track_ids.clear();
    feature_ids.clear();

    /* Commit camera using known K and computed R and t. */
    {
        CameraPose& pose = this->viewports->at(view_id).pose;
        pose = temp_camera;
        pose.R = ransac_result.pose.delete_col(3);
        pose.t = ransac_result.pose.col(3);

        if (this->opts.verbose_output)
        {
            std::cout << "Reconstructed camera "
                << view_id << " with focal length "
                << pose.get_focal_length() << std::endl;
        }
    }

    if (this->survey_points != nullptr && !registered)
        this->try_registration();

    return true;
}

/* ---------------------------------------------------------------- */

void
Incremental::try_registration () {
    std::vector<math::Vec3d> p0;
    std::vector<math::Vec3d> p1;

    for (std::size_t i = 0; i < this->survey_points->size(); ++i)
    {
        SurveyPoint const& survey_point = this->survey_points->at(i);

        std::vector<math::Vec2f> pos;
        std::vector<CameraPose const*> poses;
        for (std::size_t j = 0; j < survey_point.observations.size(); ++j)
        {
            SurveyObservation const& obs = survey_point.observations[j];
            int const view_id = obs.view_id;
            if (!this->viewports->at(view_id).pose.is_valid())
                continue;

            pos.push_back(obs.pos);
            poses.push_back(&this->viewports->at(view_id).pose);
        }

        if (pos.size() < 2)
            continue;

        p0.push_back(triangulate_track(pos, poses));
        p1.push_back(survey_point.pos);
    }

    if (p0.size() < 3)
        return;

    /* Determine transformation. */
    math::Matrix3d R;
    double s;
    math::Vec3d t;
    if (!math::determine_transform(p0, p1, &R, &s, &t))
        return;

    /* Transform every camera. */
    for (std::size_t i = 0; i < this->viewports->size(); ++i)
    {
        Viewport& view = this->viewports->at(i);
        CameraPose& pose = view.pose;
        if (!pose.is_valid())
            continue;

        pose.t = -pose.R * R.transposed() * t + pose.t * s;
        pose.R = pose.R * R.transposed();
    }

    /* Transform every point. */
    for (std::size_t i = 0; i < this->tracks->size(); ++i)
    {
        Track& track = this->tracks->at(i);
        if (!track.is_valid())
            continue;

        track.pos = R * s * track.pos + t;
    }

    this->registered = true;
}

/* ---------------------------------------------------------------- */

void
Incremental::triangulate_new_tracks (int min_num_views)
{
    Triangulate::Options triangulate_opts;
    triangulate_opts.error_threshold = this->opts.new_track_error_threshold;
    triangulate_opts.angle_threshold = this->opts.min_triangulation_angle;
    triangulate_opts.min_num_views = min_num_views;

    Triangulate::Statistics stats;
    Triangulate triangulator(triangulate_opts);

    std::size_t initial_tracks_size = this->tracks->size();
    for (std::size_t i = 0; i < this->tracks->size(); ++i)
    {
        /* Skip tracks that have already been triangulated. */
        Track const& track = this->tracks->at(i);
        if (track.is_valid())
            continue;

        /*
         * Triangulate the track using all cameras. There can be more than two
         * cameras if the track was rejected in previous triangulation attempts.
         */
        std::vector<math::Vec2f> pos;
        std::vector<CameraPose const*> poses;
        std::vector<std::size_t> view_ids;
        std::vector<std::size_t> feature_ids;
        for (std::size_t j = 0; j < track.features.size(); ++j)
        {
            int const view_id = track.features[j].view_id;
            if (!this->viewports->at(view_id).pose.is_valid())
                continue;
            int const feature_id = track.features[j].feature_id;
            pos.push_back(this->viewports->at(view_id)
                .features.positions[feature_id]);
            poses.push_back(&this->viewports->at(view_id).pose);
            view_ids.push_back(view_id);
            feature_ids.push_back(feature_id);
        }

        /* Skip tracks with too few valid cameras. */
        if ((int)poses.size() < min_num_views)
            continue;

        /* Accept track if triangulation was successful. */
        std::vector<std::size_t> outlier;
        math::Vec3d track_pos;
        if (!triangulator.triangulate(poses, pos, &track_pos, &stats, &outlier))
            continue;
        this->tracks->at(i).pos = track_pos;

        /* Check if track contains outliers */
        if (outlier.size() == 0)
            continue;

        /* Split outliers from track and generate new track */
        Track & inlier_track = this->tracks->at(i);
        Track outlier_track;
        outlier_track.invalidate();
        outlier_track.color = inlier_track.color;
        for (std::size_t i = 0; i < outlier.size(); ++i)
        {
            int const view_id = view_ids[outlier[i]];
            int const feature_id = feature_ids[outlier[i]];
            /* Remove outlier from inlier track */
            inlier_track.remove_view(view_id);
            /* Add features to new track */
            outlier_track.features.emplace_back(view_id, feature_id);
            /* Change TrackID in viewports */
            this->viewports->at(view_id).track_ids[feature_id] =
                this->tracks->size();
        }
        this->tracks->push_back(outlier_track);
    }

    if (this->opts.verbose_output)
    {
        triangulator.print_statistics(stats, std::cout);
        std::cout << "  Splitted " << this->tracks->size()
            - initial_tracks_size << " new tracks." << std::endl;
    }
}

/* ---------------------------------------------------------------- */

void
Incremental::bundle_adjustment_full (void)
{
    this->bundle_adjustment_intern(-1);
}

/* ---------------------------------------------------------------- */

void
Incremental::bundle_adjustment_single_cam (int view_id)
{
    if (view_id < 0 || std::size_t(view_id) >= this->viewports->size()
        || !this->viewports->at(view_id).pose.is_valid())
        throw std::invalid_argument("Invalid view ID");
    this->bundle_adjustment_intern(view_id);
}

/* ---------------------------------------------------------------- */

void
Incremental::bundle_adjustment_points_only (void)
{
    this->bundle_adjustment_intern(-2);
}

/* ---------------------------------------------------------------- */

void
Incremental::bundle_adjustment_intern (int single_camera_ba)
{
    ba::BundleAdjustment::Options ba_opts;
    ba_opts.fixed_intrinsics = this->opts.ba_fixed_intrinsics;
    ba_opts.verbose_output = this->opts.verbose_ba;
    if (single_camera_ba >= 0)
        ba_opts.bundle_mode = ba::BundleAdjustment::BA_CAMERAS;
    else if (single_camera_ba == -2)
        ba_opts.bundle_mode = ba::BundleAdjustment::BA_POINTS;
    else if (single_camera_ba == -1)
        ba_opts.bundle_mode = ba::BundleAdjustment::BA_CAMERAS_AND_POINTS;
    else
        throw std::invalid_argument("Invalid BA mode selection");

    /* Convert camera to BA data structures. */
    std::vector<ba::Camera> ba_cameras;
    std::vector<int> ba_cameras_mapping(this->viewports->size(), -1);
    for (std::size_t i = 0; i < this->viewports->size(); ++i)
    {
        if (single_camera_ba >= 0 && int(i) != single_camera_ba)
            continue;

        Viewport const& view = this->viewports->at(i);
        CameraPose const& pose = view.pose;
        if (!pose.is_valid())
            continue;

        ba::Camera cam;
        cam.focal_length = pose.get_focal_length();
        std::copy(pose.t.begin(), pose.t.end(), cam.translation);
        std::copy(pose.R.begin(), pose.R.end(), cam.rotation);
        std::copy(view.radial_distortion,
            view.radial_distortion + 2, cam.distortion);
        ba_cameras_mapping[i] = ba_cameras.size();
        ba_cameras.push_back(cam);
    }

    /* Convert tracks and observations to BA data structures. */
    std::vector<ba::Observation> ba_points_2d;
    std::vector<ba::Point3D> ba_points_3d;
    std::vector<int> ba_tracks_mapping(this->tracks->size(), -1);
    for (std::size_t i = 0; i < this->tracks->size(); ++i)
    {
        Track const& track = this->tracks->at(i);
        if (!track.is_valid())
            continue;

        /* Add corresponding 3D point to BA. */
        ba::Point3D point;
        std::copy(track.pos.begin(), track.pos.end(), point.pos);
        ba_tracks_mapping[i] = ba_points_3d.size();
        ba_points_3d.push_back(point);

        /* Add all observations to BA. */
        for (std::size_t j = 0; j < track.features.size(); ++j)
        {
            int const view_id = track.features[j].view_id;
            if (!this->viewports->at(view_id).pose.is_valid())
                continue;
            if (single_camera_ba >= 0 && view_id != single_camera_ba)
                continue;

            int const feature_id = track.features[j].feature_id;
            Viewport const& view = this->viewports->at(view_id);
            math::Vec2f const& f2d = view.features.positions[feature_id];

            ba::Observation point;
            std::copy(f2d.begin(), f2d.end(), point.pos);
            point.camera_id = ba_cameras_mapping[view_id];
            point.point_id = ba_tracks_mapping[i];
            ba_points_2d.push_back(point);
        }
    }

    for (std::size_t i = 0; registered && i < this->survey_points->size(); ++i)
    {
        SurveyPoint const& survey_point = this->survey_points->at(i);

        /* Add corresponding 3D point to BA. */
        ba::Point3D point;
        std::copy(survey_point.pos.begin(), survey_point.pos.end(), point.pos);
        point.is_constant = true;
        ba_points_3d.push_back(point);

        /* Add all observations to BA. */
        for (std::size_t j = 0; j < survey_point.observations.size(); ++j)
        {
            SurveyObservation const& obs = survey_point.observations[j];
            int const view_id = obs.view_id;
            if (!this->viewports->at(view_id).pose.is_valid())
                continue;
            if (single_camera_ba >= 0 && view_id != single_camera_ba)
                continue;

            ba::Observation point;
            std::copy(obs.pos.begin(), obs.pos.end(), point.pos);
            point.camera_id = ba_cameras_mapping[view_id];
            point.point_id = ba_points_3d.size() - 1;
            ba_points_2d.push_back(point);
        }
    }

    /* Run bundle adjustment. */
    ba::BundleAdjustment ba(ba_opts);
    ba.set_cameras(&ba_cameras);
    ba.set_points(&ba_points_3d);
    ba.set_observations(&ba_points_2d);
    ba.optimize();
    ba.print_status();

    /* Transfer cameras back to SfM data structures. */
    std::size_t ba_cam_counter = 0;
    for (std::size_t i = 0; i < this->viewports->size(); ++i)
    {
        if (ba_cameras_mapping[i] == -1)
            continue;

        Viewport& view = this->viewports->at(i);
        CameraPose& pose = view.pose;
        ba::Camera const& cam = ba_cameras[ba_cam_counter];

        if (this->opts.verbose_output && !this->opts.ba_fixed_intrinsics)
        {
            std::cout << "Camera " << std::setw(3) << i
                << ", focal length: "
                << util::string::get_fixed(pose.get_focal_length(), 5)
                << " -> "
                << util::string::get_fixed(cam.focal_length, 5)
                << ", distortion: "
                << util::string::get_fixed(cam.distortion[0], 5) << " "
                << util::string::get_fixed(cam.distortion[1], 5)
                << std::endl;
        }

        std::copy(cam.translation, cam.translation + 3, pose.t.begin());
        std::copy(cam.rotation, cam.rotation + 9, pose.R.begin());
        std::copy(cam.distortion, cam.distortion + 2, view.radial_distortion);
        pose.set_k_matrix(cam.focal_length, 0.0, 0.0);
        ba_cam_counter += 1;
    }

    /* Exit if single camera BA is used. */
    if (single_camera_ba >= 0)
        return;

    /* Transfer tracks back to SfM data structures. */
    std::size_t ba_track_counter = 0;
    for (std::size_t i = 0; i < this->tracks->size(); ++i)
    {
        Track& track = this->tracks->at(i);
        if (!track.is_valid())
            continue;

        ba::Point3D const& point = ba_points_3d[ba_track_counter];
        std::copy(point.pos, point.pos + 3, track.pos.begin());
        ba_track_counter += 1;
    }
}

/* ---------------------------------------------------------------- */

void
Incremental::invalidate_large_error_tracks (void)
{
    /* Iterate over all tracks and sum reprojection error. */
    std::vector<std::pair<double, std::size_t> > all_errors;
    std::size_t num_valid_tracks = 0;
    for (std::size_t i = 0; i < this->tracks->size(); ++i)
    {
        if (!this->tracks->at(i).is_valid())
            continue;

        num_valid_tracks += 1;
        math::Vec3f const& pos3d = this->tracks->at(i).pos;
        FeatureReferenceList const& ref = this->tracks->at(i).features;

        double total_error = 0.0f;
        int num_valid = 0;
        for (std::size_t j = 0; j < ref.size(); ++j)
        {
            /* Get pose and 2D position of feature. */
            int view_id = ref[j].view_id;
            int feature_id = ref[j].feature_id;

            Viewport const& viewport = this->viewports->at(view_id);
            CameraPose const& pose = viewport.pose;
            if (!pose.is_valid())
                continue;

            math::Vec2f const& pos2d = viewport.features.positions[feature_id];

            /* Project 3D feature and compute reprojection error. */
            math::Vec3d x = pose.R * pos3d + pose.t;
            math::Vec2d x2d(x[0] / x[2], x[1] / x[2]);
            double r2 = x2d.square_norm();
            x2d *= (1.0 + r2 * (viewport.radial_distortion[0]
                + viewport.radial_distortion[1] * r2))
                * pose.get_focal_length();
            total_error += (pos2d - x2d).square_norm();
            num_valid += 1;
        }
        total_error /= static_cast<double>(num_valid);
        all_errors.push_back(std::pair<double, int>(total_error, i));
    }

    if (num_valid_tracks < 2)
        return;

    /* Find the 1/2 percentile. */
    std::size_t const nth_position = all_errors.size() / 2;
    std::nth_element(all_errors.begin(),
        all_errors.begin() + nth_position, all_errors.end());
    double const square_threshold = all_errors[nth_position].first
        * this->opts.track_error_threshold_factor;

    /* Delete all tracks with errors above the threshold. */
    int num_deleted_tracks = 0;
    for (std::size_t i = nth_position; i < all_errors.size(); ++i)
    {
        if (all_errors[i].first > square_threshold)
        {
            this->tracks->at(all_errors[i].second).invalidate();
            num_deleted_tracks += 1;
        }
    }

    if (this->opts.verbose_output)
    {
        float percent = 100.0f * static_cast<float>(num_deleted_tracks)
            / static_cast<float>(num_valid_tracks);
        std::cout << "Deleted " << num_deleted_tracks
            << " of " << num_valid_tracks << " tracks ("
            << util::string::get_fixed(percent, 2)
            << "%) above a threshold of "
            << std::sqrt(square_threshold) << "." << std::endl;
    }
}

/* ---------------------------------------------------------------- */

void
Incremental::normalize_scene (void)
{
    this->registered = false;

    /* Compute AABB for all camera centers. */
    math::Vec3d aabb_min(std::numeric_limits<double>::max());
    math::Vec3d aabb_max(-std::numeric_limits<double>::max());
    math::Vec3d camera_mean(0.0);
    int num_valid_cameras = 0;
    for (std::size_t i = 0; i < this->viewports->size(); ++i)
    {
        CameraPose const& pose = this->viewports->at(i).pose;
        if (!pose.is_valid())
            continue;

        math::Vec3d center = -(pose.R.transposed() * pose.t);
        for (int j = 0; j < 3; ++j)
        {
            aabb_min[j] = std::min(center[j], aabb_min[j]);
            aabb_max[j] = std::max(center[j], aabb_max[j]);
        }
        camera_mean += center;
        num_valid_cameras += 1;
    }

    /* Compute scale and translation. */
    double scale = 10.0 / (aabb_max - aabb_min).maximum();
    math::Vec3d trans = -(camera_mean / static_cast<double>(num_valid_cameras));

    /* Transform every point. */
    for (std::size_t i = 0; i < this->tracks->size(); ++i)
    {
        if (!this->tracks->at(i).is_valid())
            continue;

        this->tracks->at(i).pos = (this->tracks->at(i).pos + trans) * scale;
    }

    /* Transform every camera. */
    for (std::size_t i = 0; i < this->viewports->size(); ++i)
    {
        CameraPose& pose = this->viewports->at(i).pose;
        if (!pose.is_valid())
            continue;
        pose.t = pose.t * scale - pose.R * trans * scale;
    }
}

/* ---------------------------------------------------------------- */

void
Incremental::print_registration_error (void) const
{
    double sum = 0;
    int num_points = 0;
    for (std::size_t i = 0; i < this->survey_points->size(); ++i)
    {
        SurveyPoint const& survey_point = this->survey_points->at(i);

        std::vector<math::Vec2f> pos;
        std::vector<CameraPose const*> poses;
        for (std::size_t j = 0; j < survey_point.observations.size(); ++j)
        {
            SurveyObservation const& obs = survey_point.observations[j];
            int const view_id = obs.view_id;
            if (!this->viewports->at(view_id).pose.is_valid())
                continue;

            pos.push_back(obs.pos);
            poses.push_back(&this->viewports->at(view_id).pose);
        }

        if (pos.size() < 2)
            continue;

        math::Vec3d recon = triangulate_track(pos, poses);
        sum += (survey_point.pos - recon).square_norm();
        num_points += 1;
    }

    if (num_points > 0)
    {
        double mse = sum / num_points;
        std::cout << "Reconstructed " << num_points
            << " survey points with a MSE of " << mse << std::endl;
    }
    else
    {
        std::cout << "Failed to reconstruct all survey points." << std::endl;
    }
}

/* ---------------------------------------------------------------- */

mve::Bundle::Ptr
Incremental::create_bundle (void) const
{
    if (this->opts.verbose_output && this->registered)
        this->print_registration_error();

    /* Create bundle data structure. */
    mve::Bundle::Ptr bundle = mve::Bundle::create();
    {
        /* Populate the cameras in the bundle. */
        mve::Bundle::Cameras& bundle_cams = bundle->get_cameras();
        bundle_cams.resize(this->viewports->size());
        for (std::size_t i = 0; i < this->viewports->size(); ++i)
        {
            mve::CameraInfo& cam = bundle_cams[i];
            Viewport const& viewport = this->viewports->at(i);
            CameraPose const& pose = viewport.pose;
            if (!pose.is_valid())
            {
                cam.flen = 0.0f;
                continue;
            }

            cam.flen = static_cast<float>(pose.get_focal_length());
            cam.ppoint[0] = pose.K[2] + 0.5f;
            cam.ppoint[1] = pose.K[5] + 0.5f;
            std::copy(pose.R.begin(), pose.R.end(), cam.rot);
            std::copy(pose.t.begin(), pose.t.end(), cam.trans);
            cam.dist[0] = viewport.radial_distortion[0]
                * MATH_POW2(pose.get_focal_length());
            cam.dist[1] = viewport.radial_distortion[1]
                * MATH_POW2(pose.get_focal_length());
        }

        /* Populate the features in the Bundle. */
        mve::Bundle::Features& bundle_feats = bundle->get_features();
        bundle_feats.reserve(this->tracks->size());
        for (std::size_t i = 0; i < this->tracks->size(); ++i)
        {
            Track const& track = this->tracks->at(i);
            if (!track.is_valid())
                continue;

            /* Copy position and color of the track. */
            bundle_feats.push_back(mve::Bundle::Feature3D());
            mve::Bundle::Feature3D& f3d = bundle_feats.back();
            std::copy(track.pos.begin(), track.pos.end(), f3d.pos);
            f3d.color[0] = track.color[0] / 255.0f;
            f3d.color[1] = track.color[1] / 255.0f;
            f3d.color[2] = track.color[2] / 255.0f;
            f3d.refs.reserve(track.features.size());
            for (std::size_t j = 0; j < track.features.size(); ++j)
            {
                /* For each reference copy view ID, feature ID and 2D pos. */
                f3d.refs.push_back(mve::Bundle::Feature2D());
                mve::Bundle::Feature2D& f2d = f3d.refs.back();
                f2d.view_id = track.features[j].view_id;
                f2d.feature_id = track.features[j].feature_id;

                FeatureSet const& features
                    = this->viewports->at(f2d.view_id).features;
                math::Vec2f const& f2d_pos
                    = features.positions[f2d.feature_id];
                std::copy(f2d_pos.begin(), f2d_pos.end(), f2d.pos);
            }
        }
    }

    return bundle;
}

SFM_BUNDLER_NAMESPACE_END
SFM_NAMESPACE_END
