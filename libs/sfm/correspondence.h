/*
 * Representation and routines for 2D-2D and 2D-3D correspondences.
 * Written by Simon Fuhrmann.
 */

#ifndef SFM_CORRESPONDENCE_HEADER
#define SFM_CORRESPONDENCE_HEADER

#include <vector>

#include "math/matrix.h"
#include "sfm/defines.h"

SFM_NAMESPACE_BEGIN

struct Correspondence;
typedef std::vector<Correspondence> Correspondences;

struct Correspondence2D3D;
typedef std::vector<Correspondence2D3D> Correspondences2D3D;

/** The IDs of a matching feature pair in two images. */
typedef std::pair<int, int> CorrespondenceIndex;
/** A list of all matching feature pairs in two images. */
typedef std::vector<CorrespondenceIndex> CorrespondenceIndices;

/**
 * Two image coordinates which correspond to each other in terms of observing
 * the same point in the scene.
 * TODO: Rename this to Correspondence2D2D.
 */
struct Correspondence
{
    double p1[2];
    double p2[2];
};

/**
 * A 3D point and an image coordinate which correspond to each other in terms
 * of the image observing this 3D point in the scene.
 */
struct Correspondence2D3D
{
    double p3d[3];
    double p2d[2];
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

/**
 * Computes two transformations for the 2D-3D correspondences such that
 * mean of the points is zero and the points fit in the unit squre/cube.
 */
void
compute_normalization (Correspondences2D3D const& correspondences,
    math::Matrix<double, 3, 3>* transform_2d,
    math::Matrix<double, 4, 4>* transform_3d);

/**
 * Applies the normalization to all correspondences.
 */
void
apply_normalization (math::Matrix<double, 3, 3> const& transform_2d,
    math::Matrix<double, 4, 4> const& transform_3d,
    Correspondences2D3D* correspondences);

SFM_NAMESPACE_END

#endif  // SFM_CORRESPONDENCE_HEADER
