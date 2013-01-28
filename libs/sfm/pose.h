/*
 * Estimation of pose between images.
 * Written by Simon Fuhrmann.
 *
 * Pose between two cameras is defined by the fundamental matrix. In the
 * calibrated case, where the cameraa internal parameters (focal length,
 * principal point) are known, pose can be described with the essential
 * matrix.
 *
 * The fundamental matrix can be computed from eight point correspondences
 * in the images, using the 8-point algorithm. It is also possible to compute
 * the fundamental matrix from seven point correspondences by enforcing
 * further constraints -- the 7-point algorithm.
 * If the camera calibration is know, the essential matrix can be computed
 * from as few as five point correspondences -- the 5-point algorithm.
 *
 * Camera matrix can be extracted from the fundamental matrix as described
 * in [Sect 9.5.3, Hartley, Zisserman, 2004] or from the essential matrix
 * as described in [Sect 9.6.2, Hartley, Zisserman, 2004].
 *
 * Properties of the Fundamental Matrix:
 * - Rank 2 homogenous matrix with 7 degrees of freedom, det(F) = 0.
 * - Relates images points x, x' in two cameras: x'^T F x = 0.
 * - If F is the fundamental matrix for camera pair (P,P'), the transpose
 *   F^T is the fundamental matrix for camera pair (P',P).
 *
 * Properties of the Essential Matrix:
 * - Rank 2 homogenous matrix with 5 degrees of freedom, det(E) = 0.
 * - Relation to Fundamental matrix: E = K'^T F K.
 * - Relates normalized image points x, x' in two cameras: x'^T E x = 0.
 *   Normalized image point x := K^-1 x* with x* (unnormalized) image point.
 * - Two equal singular vales, the third one is zero.
 */
#ifndef SFM_POSE_HEADER
#define SFM_POSE_HEADER

#include <vector>

#include "math/matrix.h"

#include "defines.h"

SFM_NAMESPACE_BEGIN

typedef math::Matrix<double, 8, 3> Eight2DPoints;
typedef math::Matrix<double, 7, 3> Seven2DPoints;
typedef math::Matrix<double, 5, 3> Five2DPoints;
typedef math::Matrix<double, 3, 3> FundamentalMatrix;
typedef math::Matrix<double, 3, 3> EssentialMatrix;

/**
 * Algorithm to compute the fundamental matrix from 8 point correspondences.
 * The implementation closely follows [Sect. 11.2, Hartley, Zisserman, 2004].
 */
bool
pose_8_point (Eight2DPoints const& points_view_1,
    Eight2DPoints const& points_view_2,
    FundamentalMatrix* result);

// TODO: Useful to have 8-point without constraint enforcement? The enforcement
// costs one SVD. Camera matrices can be extracted in case of normalized
// input coordinates and Essential Matrix contrtains in one step.

/**
 * NOT YET IMPLEMENTED.
 * Algorithm to compute the fundamental matrix from 7 point correspondences.
 * The algorithm returns one or three possible solutions.
 * The implementation follows [Sect. 11.2, 11.1.2, Hartley, Zisserman, 2004].
 */
bool
pose_7_point (Seven2DPoints const& points_view_1,
    Seven2DPoints const& points_view_2,
    std::vector<FundamentalMatrix>* result);

/**
 * NOT YET IMPLEMENTED.
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

/**
 * NOT YET IMPLEMENTED.
 * Retrieves the camera matrices from the essential matrix.
 * This routine recovers P' = [M|m] for the second camera where
 * the first camera is given in canonical form P = [I|0].
 */


/**
 * Computes a transformation matrix T for a set P of 2D points in homogeneous
 * coordinates such that mean(T*P) = 0 and scale(T*P) = 1 after transformation.
 * Mean is the centroid and scale the range of the largest dimension.
 */
template <typename T, int DIM>
void
pose_find_normalization(math::Matrix<T, DIM, 3> const& points,
    math::Matrix<T, 3, 3>* transformation);

/* ---------------------------------------------------------------- */

template <typename T, int DIM>
void
pose_find_normalization(math::Matrix<T, DIM, 3> const& points,
    math::Matrix<T, 3, 3>* transformation)
{
    transformation->fill(T(0));
    math::Vector<T, 3> mean(T(0));
    math::Vector<T, 3> aabb_min(std::numeric_limits<T>::max());
    math::Vector<T, 3> aabb_max(-std::numeric_limits<T>::max());
    for (int i = 0; i < DIM; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            mean[j] += points(i, j);
            aabb_min[j] = std::min(aabb_min[j], points(i, j));
            aabb_max[j] = std::max(aabb_max[j], points(i, j));
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

#endif /* SFM_POSE_HEADER */
