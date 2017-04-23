/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef SFM_BUNDLER_COMMON_HEADER
#define SFM_BUNDLER_COMMON_HEADER

#include <string>
#include <vector>

#include "math/vector.h"
#include "util/aligned_memory.h"
#include "mve/image.h"
#include "sfm/camera_pose.h"
#include "sfm/correspondence.h"
#include "sfm/feature_set.h"
#include "sfm/sift.h"
#include "sfm/surf.h"
#include "sfm/defines.h"

SFM_NAMESPACE_BEGIN
SFM_BUNDLER_NAMESPACE_BEGIN

/* -------------------- Common Data Structures -------------------- */

/**
 * Per-viewport information.
 * Not all data is required for every step. It should be populated on demand
 * and cleaned as early as possible to keep memory consumption in bounds.
 */
struct Viewport
{
    Viewport (void);

    /** Initial focal length estimate for the image. */
    float focal_length;
    /** Radial distortion parameter. */
    float radial_distortion[2];

    /** Camera pose for the viewport. */
    CameraPose pose;

    /** The actual image data for debugging purposes. Usually nullptr! */
    mve::ByteImage::Ptr image;
    /** Per-feature information. */
    FeatureSet features;
    /** Per-feature track ID, -1 if not part of a track. */
    std::vector<int> track_ids;
};

/** The list of all viewports considered for bundling. */
typedef std::vector<Viewport> ViewportList;

/* --------------- Data Structure for Feature Tracks -------------- */

/** References a 2D feature in a specific view. */
struct FeatureReference
{
    FeatureReference (int view_id, int feature_id);

    int view_id;
    int feature_id;
};

/** The list of all feature references inside a track. */
typedef std::vector<FeatureReference> FeatureReferenceList;

/** Representation of a feature track. */
struct Track
{
    bool is_valid (void) const;
    void invalidate (void);
    void remove_view (int view_id);

    math::Vec3f pos;
    math::Vec3uc color;
    FeatureReferenceList features;
};

/** The list of all tracks. */
typedef std::vector<Track> TrackList;

/* Observation of a survey point in a specific view. */
struct SurveyObservation
{
    SurveyObservation (int view_id, float x, float y);

    int view_id;
    math::Vec2f pos;
};

/** The list of all survey point observations inside a survey point. */
typedef std::vector<SurveyObservation> SurveyObservationList;

/** Representation of a survey point. */
struct SurveyPoint
{
    math::Vec3f pos;
    SurveyObservationList observations;
};

/** The list of all survey poins. */
typedef std::vector<SurveyPoint> SurveyPointList;

/* ------------- Data Structures for Feature Matching ------------- */

/** The matching result between two views. */
struct TwoViewMatching
{
    bool operator< (TwoViewMatching const& rhs) const;

    int view_1_id;
    int view_2_id;
    CorrespondenceIndices matches;
};

/** The matching result between several pairs of views. */
typedef std::vector<TwoViewMatching> PairwiseMatching;

/* ------------------ Input/Output for Prebundle ------------------ */

/**
 * Saves the pre-bundle data to file, which records all viewport and
 * matching data necessary for incremental structure-from-motion.
 */
void
save_prebundle_to_file (ViewportList const& viewports,
    PairwiseMatching const& matching, std::string const& filename);

/**
 * Loads the pre-bundle data from file, initializing viewports and matching.
 */
void
load_prebundle_from_file (std::string const& filename,
    ViewportList* viewports, PairwiseMatching* matching);

/**
 * Loads survey points and their observations from file.
 *
 * Survey file are ASCII files that start with the signature
 * MVE_SURVEY followed by a newline, followed by the number of survey points
 * and survey point observations.
 * Each survey point is a 3D point followed by a newline. Each survey point
 * observation is a line starting with the index of the survey point, followed
 * by the view id an the 2D location within the image. The (x, y) coordinates
 * have to be normalized such that the center of the image is (0, 0) and the
 * larger image dimension is one. This means that all image coordinates are
 * between (-0.5,-0.5) and (0.5, 0.5)
 *
 * MVE_SURVEY
 * <num_points> <num_observations>
 * <survey_point> // x y z
 * ...
 * <survey_point_observation> // survey_point_id view_id x y
 * ...
 */
void
load_survey_from_file (std::string const& filename,
    SurveyPointList* survey_points);

/* ------------------------ Implementation ------------------------ */

inline
FeatureReference::FeatureReference (int view_id, int feature_id)
    : view_id(view_id)
    , feature_id(feature_id)
{
}

inline
SurveyObservation::SurveyObservation (int view_id, float x, float y)
    : view_id(view_id)
    , pos(x, y)
{
}

inline bool
TwoViewMatching::operator< (TwoViewMatching const& rhs) const
{
    return this->view_1_id == rhs.view_1_id
        ? this->view_2_id < rhs.view_2_id
        : this->view_1_id < rhs.view_1_id;
}

inline
Viewport::Viewport (void)
    : focal_length(0.0f)
{
    std::fill(this->radial_distortion, this->radial_distortion + 2, 0.0f);
}

inline bool
Track::is_valid (void) const
{
    return !std::isnan(this->pos[0]);
}

SFM_BUNDLER_NAMESPACE_END
SFM_NAMESPACE_END

#endif /* SFM_BUNDLER_COMMON_HEADER */
