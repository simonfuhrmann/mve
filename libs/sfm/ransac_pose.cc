#include <algorithm>
#include <iostream>
#include <set>
#include <stdexcept>

#include "util/system.h"
#include "math/algo.h"
#include "sfm/ransac_pose.h"
#include "sfm/pose.h"

SFM_NAMESPACE_BEGIN

RansacPose::Options::Options (void)
    : max_iterations(100)
    , threshold(1e-3)
    , verbose_output(false)
{
}

RansacPose::RansacPose (Options const& options)
    : opts(options)
{
}

void
RansacPose::estimate (Correspondences2D3D const& corresp, Result* result)
{
    if (this->opts.verbose_output)
    {
        std::cout << "RANSAC-6: Running for " << this->opts.max_iterations
            << " iterations..." << std::endl;
    }

    std::vector<int> inliers;
    inliers.reserve(corresp.size());
    for (int iteration = 0; iteration < this->opts.max_iterations; ++iteration)
    {
        math::Matrix<double, 3, 4> p_matrix;
        this->estimate_6_point(corresp, &p_matrix);
        this->find_inliers(corresp, p_matrix, &inliers);
        if (inliers.size() > result->inliers.size())
        {
            if (this->opts.verbose_output)
            {
                std::cout << "RANSAC-6: Iteration " << iteration
                    << ", inliers " << inliers.size() << " ("
                    << (100.0 * inliers.size() / corresp.size())
                    << "%)" << std::endl;
            }

            result->p_matrix = p_matrix;
            std::swap(result->inliers, inliers);
            inliers.reserve(corresp.size());
        }
    }
}

void
RansacPose::estimate_6_point (Correspondences2D3D const& corresp,
    math::Matrix<double, 3, 4>* p_matrix)
{
    if (corresp.size() < 6)
        throw std::invalid_argument("At least 6 correspondences required");

    /* Draw 6 unique random numbers. */
    std::set<int> result;
    while (result.size() < 6)
        result.insert(util::system::rand_int() % corresp.size());

    /* Create list of the six selected correspondences. */
    Correspondences2D3D selection(6);
    std::set<int>::const_iterator iter = result.begin();
    for (int i = 0; i < 6; ++i, ++iter)
        selection[i] = corresp[*iter];

    /* Obtain pose from the selection. */
    pose_from_2d_3d_correspondences(selection, p_matrix);
}

void
RansacPose::find_inliers (Correspondences2D3D const& corresp,
    math::Matrix<double, 3, 4> const& p_matrix,
    std::vector<int>* result)
{
    result->resize(0);
    double const square_threshold = MATH_POW2(this->opts.threshold);
    for (std::size_t i = 0; i < corresp.size(); ++i)
    {
        Correspondence2D3D const& c = corresp[i];
        math::Vec4d p3d(c.p3d[0], c.p3d[1], c.p3d[2], 1.0);
        math::Vec3d p2d = p_matrix * p3d;
        double const square_distance
            = MATH_POW2(p2d[0] / p2d[2] - corresp[i].p2d[0])
            + MATH_POW2(p2d[1] / p2d[2] - corresp[i].p2d[1]);
        if (square_distance < square_threshold)
            result->push_back(i);
    }
}

SFM_NAMESPACE_END
