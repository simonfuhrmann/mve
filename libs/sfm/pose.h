/*
 * Data structure and routines for camera pose.
 * Written by Simon Fuhrmann.
 */
#ifndef SFM_POSE_HEADER
#define SFM_POSE_HEADER

#include "math/matrix.h"

#include "defines.h"
#include "correspondence.h"

SFM_NAMESPACE_BEGIN

/**
 * The camera pose is P = K [R | t].
 * K is the calibration matrix, R a rotation matrix and t a translation.
 *
 *       | f  0  px |    The calibration matrix contains the focal length f,
 *   K = | 0  f  py |    and the principal point px and py.
 *       | 0  0   1 |
 *
 * For pose estimation, the calibration matrix is assumed to be known. This
 * might not be the case, but even a good guess of the focal length and the
 * principal point set to the image center can produce reasonably good
 * results so that bundle adjustment can recover better parameters.
 */
struct CameraPose
{
    math::Matrix<double, 3, 3> K;
    math::Matrix<double, 3, 3> R;
    math::Vector<double, 3> t;

    void init_canonical_form (void);
    void fill_p_matrix (math::Matrix<double, 3, 4>* result) const;
    void set_k_matrix (double flen, double px, double py);
    void set_from_p_and_known_k (math::Matrix<double, 3, 4> const& p_matrix);
};

/**
 * Estimates the camera pose from 2D-3D correspondences, i.e. correspondences
 * between 3D world coordinates and 2D image coordiantes. At least six such
 * correspondences are required.
 *
 * In order to improve numerical stability of the operation, the input
 * correspondences, both 2D and 3D points, should be normalized by
 * removing the mean and scaling to the unit spare/cube.
 */
void
pose_from_2d_3d_correspondences (Correspondences2D3D const& corresp,
    math::Matrix<double, 3, 4>* p_matrix);

SFM_NAMESPACE_END

#endif /* SFM_POSE_HEADER */
