/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <cmath>

#include "math/functions.h"
#include "sfm/ransac.h"

SFM_NAMESPACE_BEGIN

int
compute_ransac_iterations (double inlier_ratio,
    int num_samples,
    double desired_success_rate)
{
    double prob_all_good = math::fastpow(inlier_ratio, num_samples);
    double num_iterations = std::log(1.0 - desired_success_rate)
        / std::log(1.0 - prob_all_good);
    return static_cast<int>(math::round(num_iterations));
}

SFM_NAMESPACE_END
