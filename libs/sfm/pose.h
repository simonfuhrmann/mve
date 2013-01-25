/*
 * Estimation of pose between images.
 * Written by Simon Fuhrmann.
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

#if 0
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
