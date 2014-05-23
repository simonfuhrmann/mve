/*
 * Data structure and routines for camera pose.
 * Written by Simon Fuhrmann.
 */
#ifndef SFM_POSE_HEADER
#define SFM_POSE_HEADER

#include "math/matrix.h"
#include "sfm/defines.h"
#include "sfm/correspondence.h"

SFM_NAMESPACE_BEGIN

/**
 * The camera pose is the 3x4 matrix P = K [R | t]. K is the 3x3 calibration
 * matrix, R a 3x3 rotation matrix and t a 3x1 translation vector.
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
    CameraPose (void);

    math::Matrix<double, 3, 3> K;
    math::Matrix<double, 3, 3> R;
    math::Vector<double, 3> t;

    /** Initializes R with identity and t with zeros. */
    void init_canonical_form (void);
    /** Returns the P matrix as the product K [R | t]. */
    void fill_p_matrix (math::Matrix<double, 3, 4>* result) const;
    /** Initializes the K matrix from focal length and principal point. */
    void set_k_matrix (double flen, double px, double py);
    /** Initializes the R and t from P and known K (must already be set). */
    void set_from_p_and_known_k (math::Matrix<double, 3, 4> const& p_matrix);
    /** Returns the focal length as average of x and y focal length. */
    double get_focal_length (void) const;
    /** Returns the camera position (requires valid camera). */
    void fill_camera_pos (math::Vector<double, 3>* camera_pos) const;
    /** Returns true if K matrix is valid (non-zero focal length). */
    bool is_valid (void) const;
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

/**
 * Decomposes the P-matrix into intrinsic and extrinsic parameters.
 * This decomposes P into K [R|t] using QR decomposition.
 */
void
pose_from_p_matrix (math::Matrix<double, 3, 4> const& p_matrix,
    CameraPose* pose);

/**
 * Computes the optimal rotation matrix R for given matrix A such that the
 * squared frobenius norm ||A-R||^2 is minimized subject to R R^T = 1
 * and det(R) = 1. This is equivalent to maximizing tr(A^T R) and yields
 *
 *   R = U C V^T
 *
 * where U S V^T is the SVD of A and C a matrix that negates the last column
 * of V if and only if det(A) < 0. The technique is described in the paper
 *
 *   On the closed-form solution of the rotation matrix arising in
 *   computer vision problems, Andriy Myronenko and Xubo Song, 2009.
 */
math::Matrix<double, 3, 3>
matrix_optimal_rotation  (math::Matrix<double, 3, 3> const& matrix);

SFM_NAMESPACE_END

#endif /* SFM_POSE_HEADER */
