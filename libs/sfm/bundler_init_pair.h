/*
 * Bundler component that computes the initial pair for reconstruction.
 * Written by Simon Fuhrmann.
 */

#ifndef SFM_BUNDLER_INIT_PAIR_HEADER
#define SFM_BUNDLER_INIT_PAIR_HEADER

#include "sfm/ransac_homography.h"
#include "sfm/ransac_fundamental.h"
#include "sfm/fundamental.h"
#include "sfm/bundler_common.h"
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

    struct Result
    {
        int view_1_id;
        int view_2_id;
    };

public:
    explicit InitialPair (Options const& options);
    void compute (ViewportList const& viewports,
        PairwiseMatching const& matching, Result* result);

private:
    Options opts;
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
{
}

SFM_BUNDLER_NAMESPACE_END
SFM_NAMESPACE_END

#endif /* SFM_BUNDLER_INIT_PAIR_HEADER */
