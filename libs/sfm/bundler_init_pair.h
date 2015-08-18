/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef SFM_BUNDLER_INIT_PAIR_HEADER
#define SFM_BUNDLER_INIT_PAIR_HEADER

#include <vector>

#include "sfm/ransac_homography.h"
#include "sfm/ransac_fundamental.h"
#include "sfm/fundamental.h"
#include "sfm/bundler_common.h"
#include "sfm/camera_pose.h"
#include "sfm/defines.h"

SFM_NAMESPACE_BEGIN
SFM_BUNDLER_NAMESPACE_BEGIN

/**
 * Tries to find an initial viewport pair to start the reconstruction with.
 * The implemented strategy sorts all pairwise matching results by the
 * number of matches and chooses the first pair where the matches cannot
 * be explained with a homography.
 */
class InitialPair
{
public:
    struct Options
    {
        Options (void);

        /**
         * The algorithm tries to explain the matches using a homography.
         * The homograhy is computed using RANSAC with the given options.
         */
        RansacHomography::Options homography_opts;

        /**
         * The maximum percentage of homography inliers. Default is 0.6.
         */
        float max_homography_inliers;

        /**
         * Produce status messages on the console.
         */
        bool verbose_output;
    };

    /**
     * The resulting initial pair with view IDs and relative camera pose.
     * If no initial pair could be found, both view IDs are set to -1.
     */
    struct Result
    {
        int view_1_id;
        int view_2_id;
        CameraPose view_1_pose;
        CameraPose view_2_pose;
    };

public:
    explicit InitialPair (Options const& options);
    /** Initializes the module with viewport and track information. */
    void initialize (ViewportList const& viewports, TrackList const& tracks);
    /** Finds a suitable initial pair and reconstructs the pose. */
    void compute_pair (Result* result);
    /** Reconstructs the pose for a given intitial pair. */
    void compute_pair (int view_1_id, int view_2_id, Result* result);

private:
    struct CandidatePair
    {
        int view_1_id;
        int view_2_id;
        Correspondences2D2D matches;
        bool operator< (CandidatePair const& other) const;
    };
    typedef std::vector<CandidatePair> CandidatePairs;

private:
    float compute_homography_ratio (CandidatePair const& candidate);
    bool compute_pose (CandidatePair const& candidate,
        CameraPose* pose1, CameraPose* pose2);
    void compute_candidate_pairs (CandidatePairs* candidates);

private:
    Options opts;
    ViewportList const* viewports;
    TrackList const* tracks;
};

/* ------------------------ Implementation ------------------------ */

inline
InitialPair::Options::Options (void)
    : max_homography_inliers(0.6f)
    , verbose_output(false)
{
}

inline
InitialPair::InitialPair (Options const& options)
    : opts(options)
    , viewports(NULL)
    , tracks(NULL)
{
}

inline void
InitialPair::initialize (ViewportList const& viewports, TrackList const& tracks)
{
    this->viewports = &viewports;
    this->tracks = &tracks;
}

inline bool
InitialPair::CandidatePair::operator< (CandidatePair const& other) const
{
    return this->matches.size() < other.matches.size();
}

SFM_BUNDLER_NAMESPACE_END
SFM_NAMESPACE_END

#endif /* SFM_BUNDLER_INIT_PAIR_HEADER */
