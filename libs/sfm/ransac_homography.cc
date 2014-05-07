#include <algorithm>
#include <iostream>
#include <set>
#include <stdexcept>

#include "util/system.h"
#include "math/algo.h"
#include "math/matrix_tools.h"
#include "sfm/ransac_homography.h"
#include "sfm/matrixsvd.h"

SFM_NAMESPACE_BEGIN

RansacHomography::Options::Options (void)
    : max_iterations(1000)
    , threshold(0.01)
    , already_normalized(true)
    , verbose_output(false)
{
}

RansacHomography::RansacHomography (Options const& options)
    : opts(options)
{
}

void
RansacHomography::estimate (Correspondences const& matches, Result* result)
{
    if (this->opts.verbose_output)
    {
        std::cout << "RANSAC-H: Running for " << this->opts.max_iterations
            << " iterations..." << std::endl;
    }

    std::vector<int> inliers;
    inliers.reserve(matches.size());
    for (int iteration = 0; iteration < this->opts.max_iterations; ++iteration)
    {
        HomographyMatrix homography;
        this->compute_homography(matches, &homography);
        this->evaluate_homography(matches, homography, &inliers);
        if (inliers.size() > result->inliers.size())
        {
            if (this->opts.verbose_output)
            {
                std::cout << "RANSAC-H: Iteration " << iteration
                    << ", inliers " << inliers.size() << " ("
                    << (100.0 * inliers.size() / matches.size())
                    << "%)" << std::endl;
            }

            result->homography = homography;
            std::swap(result->inliers, inliers);
            inliers.reserve(matches.size());
        }
    }
}

void
RansacHomography::compute_homography (Correspondences const& matches,
    HomographyMatrix* homography)
{
    if (matches.size() < 4)
        throw std::invalid_argument("At least 4 matches required");

    /*
     * Draw 4 random numbers in the interval [0, matches.size() - 1]
     * without duplicates. This is done by keeping a set with drawn numbers.
     */
    std::set<int> result;
    while (result.size() < 4)
        result.insert(util::system::rand_int() % matches.size());

    Correspondences four_correspondeces(4);
    std::set<int>::const_iterator iter = result.begin();
    for (std::size_t i = 0; i < 4; ++i, ++iter)
        four_correspondeces[i] = matches[*iter];

    math::Matrix3d T1, T2, invT2;
    if (!this->opts.already_normalized)
    {
        sfm::compute_normalization(four_correspondeces, &T1, &T2);
        sfm::apply_normalization(T1, T2, &four_correspondeces);
        invT2 = math::matrix_inverse(T2);
    }

    sfm::homography_dlt(four_correspondeces, homography);
    *homography /= (*homography)[8];

    if (!this->opts.already_normalized)
        *homography = invT2 * *homography * T1;
}

void
RansacHomography::evaluate_homography (Correspondences const& matches,
    HomographyMatrix const& homography, std::vector<int>* inliers)
{
    double const square_threshold = MATH_POW2(this->opts.threshold);
    inliers->resize(0);
    for (std::size_t i = 0; i < matches.size(); i++)
    {
        Correspondence const& match = matches[i];
        double error = sfm::symmetric_transfer_error(homography, match);
        if (error < square_threshold)
            inliers->push_back(i);
    }
}

SFM_NAMESPACE_END