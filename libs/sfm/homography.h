#ifndef SFM_HOMOGRAPHY_HEADER
#define SFM_HOMOGRAPHY_HEADER

#include "math/matrix.h"
#include "sfm/defines.h"
#include "sfm/correspondence.h"

SFM_NAMESPACE_BEGIN

typedef math::Matrix3d HomographyMatrix;

/**
 * Direct linear transformation algorithm to compute the homography matrix from
 * image correspondences. This algorithm computes the least squares solution for
 * the homography matrix from at least 4 correspondences.
 */
bool
homography_dlt (Correspondences const& matches, HomographyMatrix* result);

/**
 * Computes the symmetric transfer error for an image correspondence given the
 * homography matrix between two views.
 */
double
symmetric_transfer_error(HomographyMatrix const& homography,
    Correspondence const& match);

SFM_NAMESPACE_END

#endif // SFM_HOMOGRAPHY_HEADER
