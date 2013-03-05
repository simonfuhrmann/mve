#include <algorithm>
#include <cstdlib> // for std::rand()
#include <iostream> // TMP
#include <set>
#include <stdexcept>

#include "math/algo.h"
#include "poseransac.h"

SFM_NAMESPACE_BEGIN

PoseRansac::Options::Options (void)
    : max_iterations(100)
    , threshold(1e-3)
    , already_normalized(true)
{
}

PoseRansac::PoseRansac (Options const& options)
    : opts(options)
{
}

void
PoseRansac::estimate (Correspondences const& matches, Result* result)
{
    std::vector<int> inliers;
    inliers.reserve(matches.size());
    std::cout << "RANSAC: Running for " << this->opts.max_iterations
        << " iterations..." << std::endl;
    for (int iteration = 0; iteration < this->opts.max_iterations; ++iteration)
    {
        /* Randomly select points and estimate fundamental matrix. */
        FundamentalMatrix fundamental;
        estimate_8_point(matches, &fundamental);

        /* Find matches supporting the fundamental matrix. */
        find_inliers(matches, fundamental, &inliers);

        if (inliers.size() > result->inliers.size())
            std::cout << "RANASC: Iteration " << iteration
                << ", inliers " << inliers.size() << std::endl;

        /* Check if current selection is best so far. */
        if (inliers.size() > result->inliers.size())
        {
            result->fundamental = fundamental;
            std::swap(result->inliers, inliers);
        }
    }
}

void
PoseRansac::estimate_8_point (Correspondences const& matches,
    FundamentalMatrix* fundamental)
{
    if (matches.size() < 8)
        throw std::invalid_argument("At least 8 matches required");

    /*
     * Draw 8 random numbers in the interval [0, matches.size() - 1]
     * without duplicates. This is done by keeping a set with drawn numbers.
     */
    std::set<int> result;
    while (result.size() < 8)
        result.insert(std::rand() % matches.size());

#if 0
    {
        std::set<int>::const_iterator iter = result.begin();
        std::cout << "Drawn IDs: " << *(iter++) << " " << *(iter++) << " "
            << *(iter++) << " " << *(iter++) << " " << *(iter++) << " "
            << *(iter++) << " " << *(iter++) << " " << *(iter++)
            << " "; //std::endl;
    }
#endif

    math::Matrix<double, 3, 8> pset1, pset2;
    std::set<int>::const_iterator iter = result.begin();
    for (int i = 0; i < 8; ++i, ++iter)
    {
        Correspondence const& match = matches[*iter];
        pset1(0, i) = match.p1[0];
        pset1(1, i) = match.p1[1];
        pset1(2, i) = 1.0;
        pset2(0, i) = match.p2[0];
        pset2(1, i) = match.p2[1];
        pset2(2, i) = 1.0;
    }

    /* Compute fundamental matrix using normalized 8-point. */
    math::Matrix<double, 3, 3> T1, T2;
    if (!this->opts.already_normalized)
    {
        sfm::compute_normalization(pset1, &T1);
        sfm::compute_normalization(pset2, &T2);
        pset1 = T1 * pset1;
        pset2 = T2 * pset2;
    }

    sfm::fundamental_8_point(pset1, pset2, fundamental);
    sfm::enforce_fundamental_constraints(fundamental);

    if (!this->opts.already_normalized)
    {
        *fundamental = T2.transposed().mult(*fundamental).mult(T1);
    }
}

void
PoseRansac::find_inliers (Correspondences const& matches,
    FundamentalMatrix const& fundamental, std::vector<int>* result)
{
    result->resize(0);
    double const squared_thres = this->opts.threshold * this->opts.threshold;
    for (std::size_t i = 0; i < matches.size(); ++i)
    {
        double error = sampson_distance(fundamental, matches[i]);
        //std::cout << "Sampson error for match " << i << ": " << error << std::endl;
        if (error < squared_thres)
            result->push_back(i);
    }
}

double
PoseRansac::sampson_distance (FundamentalMatrix const& F,
    Correspondence const& m)
{
    /*
     * Computes the Sampson distance SD for a given match and fundamental
     * matrix. SD is computed as [Sect 11.4.3, Hartley, Zisserman]:
     *
     *   SD = (x'Fx)^2 / ( (Fx)_1^2 + (Fx)_2^2 + (x'F)_1^2 + (x'F)_2^2 )
     */

    double p2_F_p1 = 0.0;
    p2_F_p1 += m.p2[0] * (m.p1[0] * F[0] + m.p1[1] * F[1] + F[2]);
    p2_F_p1 += m.p2[1] * (m.p1[0] * F[3] + m.p1[1] * F[4] + F[5]);
    p2_F_p1 +=     1.0 * (m.p1[0] * F[6] + m.p1[1] * F[7] + F[8]);
    p2_F_p1 *= p2_F_p1;

    double sum = 0.0;
    sum += math::algo::fastpow(m.p1[0] * F[0] + m.p1[1] * F[1] + F[2], 2);
    sum += math::algo::fastpow(m.p1[0] * F[3] + m.p1[1] * F[4] + F[5], 2);
    sum += math::algo::fastpow(m.p2[0] * F[0] + m.p2[1] * F[3] + F[6], 2);
    sum += math::algo::fastpow(m.p2[0] * F[1] + m.p2[1] * F[4] + F[7], 2);

    return p2_F_p1 / sum;
}

SFM_NAMESPACE_END
