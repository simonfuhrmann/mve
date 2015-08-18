/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 *
 * The relation between two cameras is defined by the fundamental matrix.
 * In the calibrated case, where the camera internal parameters (focal
 * length, principal point) are known, pose can be described with the essential
 * matrix.
 *
 * The fundamental matrix can be computed from eight point correspondences
 * in the images, using the 8-point algorithm. It is also possible to compute
 * the fundamental matrix from seven point correspondences by enforcing
 * further constraints -- the 7-point algorithm.
 * If the camera calibration is know, the essential matrix can be computed
 * from as few as five point correspondences -- the 5-point algorithm.
 *
 * The input points to the N-point algorithms should be normalized such that
 * the mean of the points is zero and the points fit in the unit square. This
 * makes solving for the fundamental matrix numerically stable. The inverse
 * transformation can then applied afterwards. That is, for transformations
 * T1 and T2, the de-normalized fundamental matrix is given by F' = T2* F T1,
 * where T* is the transpose of T.
 *
 * Camera matrix can be extracted from the essential matrix as described
 * in [Sect 9.6.2, Hartley, Zisserman, 2004].
 *
 * Properties of the Fundamental matrix F:
 * - Rank 2 homogenous matrix with 7 degrees of freedom, det(F) = 0.
 * - Relates images points x, x' in two cameras: x'^T F x = 0.
 * - If F is the fundamental matrix for camera pair (P,P'), the transpose
 *   F^T is the fundamental matrix for camera pair (P',P).
 * - Two non-zero singular values.
 *
 * Properties of the Essential matrix E:
 * - Rank 2 homogenous matrix with 5 degrees of freedom, det(E) = 0.
 * - Relation to Fundamental matrix: E = K'^T F K.
 * - Relates normalized image points x, x' in two cameras: x'^T E x = 0.
 *   Normalized image point x := K^-1 x* with x* (unnormalized) image point.
 * - Two equal singular vales, the third one is zero.
 */
#ifndef SFM_FUNDAMENTAL_HEADER
#define SFM_FUNDAMENTAL_HEADER

#include <limits>
#include <vector>

#include "math/vector.h"
#include "math/matrix.h"
#include "sfm/defines.h"
#include "sfm/camera_pose.h"
#include "sfm/correspondence.h"

SFM_NAMESPACE_BEGIN

typedef math::Matrix<double, 3, 8> Eight2DPoints;
typedef math::Matrix<double, 3, 3> FundamentalMatrix;
typedef math::Matrix<double, 3, 3> EssentialMatrix;

/**
 * Algorithm to compute the fundamental or essential matrix from image
 * correspondences. This algorithm computes the least squares solution for
 * the fundamental matrix from at least 8 correspondences. The solution is
 * sensitive to outliers.
 *
 * This does not normalize the image coordinates for stability or enforces
 * constraints on the resulting matrix.
 */
bool
fundamental_least_squares (Correspondences2D2D const& points,
    FundamentalMatrix* result);

/**
 * Algorithm to compute the fundamental or essential matrix from 8 image
 * correspondences. It closely follows [Sect. 11.2, Hartley, Zisserman, 2004].
 * In case of "normalized image coordinates" (i.e. x* = K^-1 x), this code
 * computes the essential matrix.
 *
 * This does not normalize the image coordinates for stability or enforces
 * constraints on the resulting matrix.
 *
 * Note: For eight points this code computes the same result as the least
 * squares version but with fixed matrix sizes for compile time optimizations.
 */
bool
fundamental_8_point (Eight2DPoints const& points_view_1,
    Eight2DPoints const& points_view_2, FundamentalMatrix* result);

/**
 * Constraints the given matrix to have TWO NON-ZERO eigenvalues.
 * This is done using SVD: F' = USV*, F = UDV* with D = diag(a, b, 0).
 */
void
enforce_fundamental_constraints (FundamentalMatrix* matrix);

/**
 * Constraints the given matrix to have TWO EQUAL NON-ZERO eigenvalues.
 * This is done using SVD: F' = USV*, F = UDV* with D = diag(a, a, 0).
 */
void
enforce_essential_constraints (EssentialMatrix* matrix);

/**
 * Retrieves the camera matrices from the essential matrix. This routine
 * recovers P' = [R|t] for the second camera where the first camera is given
 * in canonical form P = [I|0]. The pose can be computed up to scale and a
 * four-fold ambiguity. That is, the resulting translation has length one and
 * four possible solutions are provided. In case of two cameras in the same
 * location, the rotation is still reliable but the translation is unstable.
 *
 * Each of the solutions must be tested: It is sufficient to test if a single
 * point (triangulated from a 2D-2D correspondence) is in front of both
 * cameras. Note: The resulting camera pose does not contain the K matrix.
 * Before testing the resulting poses, the K-matrix must be set!
 */
void
pose_from_essential (EssentialMatrix const& matrix,
    std::vector<CameraPose>* result);

/**
 * Computes the fundamental matrix corresponding to cam1 and cam2.
 * The function relies on the epipole to be visible in the second
 * camera, thus the cameras must not be in the same location.
 */
void
fundamental_from_pose (CameraPose const& cam1, CameraPose const& cam2,
    FundamentalMatrix* result);

/**
 * Computes the Sampson distance for an image correspondence given the
 * fundamental matrix between two views.
 */
double
sampson_distance (FundamentalMatrix const& fundamental,
    Correspondence2D2D const& match);

/**
 * Computes a transformation for 2D points in homogeneous coordinates
 * such that the mean of the points is zero and the points fit in the unit
 * square. (The thrid coordinate will still be 1 after normalization.)
 * Optimized version where number of points must be known at compile time.
 */
template <typename T, int DIM>
void
compute_normalization(math::Matrix<T, 3, DIM> const& points,
    math::Matrix<T, 3, 3>* transformation);

/* ---------------------------------------------------------------- */

#if 0  // This is not yet implemented!
typedef math::Matrix<double, 3, 7> Seven2DPoints;
typedef math::Matrix<double, 3, 5> Five2DPoints;

/**
 * Algorithm to compute the fundamental matrix from 7 point correspondences.
 * The algorithm returns one or three possible solutions.
 * The implementation follows [Sect. 11.2, 11.1.2, Hartley, Zisserman, 2004].
 */
bool
pose_7_point (Seven2DPoints const& points_view_1,
    Seven2DPoints const& points_view_2,
    std::vector<FundamentalMatrix>* result);

/**
 * Algorithm to compute the essential matrix from 5 point correspondences.
 * The algorithm returns up to ten possible solutions.
 * The literature can be found at: http://vis.uky.edu/~stewe/FIVEPOINT/
 *
 *    Recent developments on direct relative orientation,
 *    H. Stewenius, C. Engels, and D. Nister, ISPRS 2006.
 *
 */
bool
pose_5_point (Five2DPoints const& points_view_1,
    Five2DPoints const& points_view_2,
    std::vector<EssentialMatrix>* result);
#endif

/* ---------------------------------------------------------------- */

template <typename T, int DIM>
void
compute_normalization(math::Matrix<T, 3, DIM> const& points,
    math::Matrix<T, 3, 3>* transformation)
{
    math::Vector<T, 3> mean(T(0));
    math::Vector<T, 3> aabb_min(std::numeric_limits<T>::max());
    math::Vector<T, 3> aabb_max(-std::numeric_limits<T>::max());
    for (int i = 0; i < DIM; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            mean[j] += points(j, i);
            aabb_min[j] = std::min(aabb_min[j], points(j, i));
            aabb_max[j] = std::max(aabb_max[j], points(j, i));
        }
    }
    mean /= static_cast<T>(DIM);
    T norm = (aabb_max - aabb_min).maximum();
    math::Matrix<T, 3, 3>& t = *transformation;
    t[0] = T(1) / norm; t[1] = T(0);        t[2] = -mean[0] / norm;
    t[3] = T(0);        t[4] = T(1) / norm; t[5] = -mean[1] / norm;
    t[6] = T(0);        t[7] = T(0);        t[8] = T(1);
}

SFM_NAMESPACE_END

#endif // SFM_FUNDAMENTAL_HEADER
