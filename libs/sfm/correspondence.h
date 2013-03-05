#ifndef SFM_CORRESPONDENCE_HEADER
#define SFM_CORRESPONDENCE_HEADER

#include <vector>

#include "math/matrix.h"

#include "defines.h"

SFM_NAMESPACE_BEGIN

struct Correspondence;
typedef std::vector<Correspondence> Correspondences;

/**
 * Two image coordinates which correspond to each other in terms of observing
 * the same point in the scene.
 */
struct Correspondence
{
    double p1[2];
    double p2[2];
};

/**
 * Computes two transformations for the 2D points specified in the
 * correspondences such that the mean of the points is zero and the points
 * fit in the unit square.
 */
void
compute_normalization (Correspondences const& correspondences,
    math::Matrix<double, 3, 3>* transform1,
    math::Matrix<double, 3, 3>* transform2);

/**
 * Applies the normalization to all correspondences.
 */
void
apply_normalization (math::Matrix<double, 3, 3> const& transform1,
    math::Matrix<double, 3, 3> const& transform2,
    Correspondences* correspondences);

SFM_NAMESPACE_END

#endif  // SFM_CORRESPONDENCE_HEADER
