#include <limits>

#include "sfm/correspondence.h"

SFM_NAMESPACE_BEGIN

void
compute_normalization (Correspondences const& correspondences,
    math::Matrix<double, 3, 3>* transform1,
    math::Matrix<double, 3, 3>* transform2)
{
    math::Vector<double, 2> t1_mean(0.0);
    math::Vector<double, 2> t1_aabb_min(std::numeric_limits<double>::max());
    math::Vector<double, 2> t1_aabb_max(-std::numeric_limits<double>::max());
    math::Vector<double, 2> t2_mean(0.0);
    math::Vector<double, 2> t2_aabb_min(std::numeric_limits<double>::max());
    math::Vector<double, 2> t2_aabb_max(-std::numeric_limits<double>::max());

    for (std::size_t i = 0; i < correspondences.size(); ++i)
    {
        Correspondence const& p = correspondences[i];
        for (int j = 0; j < 2; ++j)
        {
            t1_mean[j] += p.p1[j];
            t1_aabb_min[j] = std::min(t1_aabb_min[j], p.p1[j]);
            t1_aabb_max[j] = std::max(t1_aabb_max[j], p.p1[j]);
            t2_mean[j] += p.p2[j];
            t2_aabb_min[j] = std::min(t2_aabb_min[j], p.p2[j]);
            t2_aabb_max[j] = std::max(t2_aabb_max[j], p.p2[j]);
        }
    }

    t1_mean /= static_cast<double>(correspondences.size());
    double t1_norm = (t1_aabb_max - t1_aabb_min).maximum();
    math::Matrix<double, 3, 3>& t1 = *transform1;
    t1.fill(0.0);
    t1[0] = 1.0 / t1_norm;
    t1[2] = -t1_mean[0] / t1_norm;
    t1[4] = 1.0 / t1_norm;
    t1[5] = -t1_mean[1] / t1_norm;
    t1[8] = 1.0;

    t2_mean /= static_cast<double>(correspondences.size());
    double t2_norm = (t2_aabb_max - t2_aabb_min).maximum();
    math::Matrix<double, 3, 3>& t2 = *transform2;
    t2.fill(0.0);
    t2[0] = 1.0 / t2_norm;
    t2[2] = -t2_mean[0] / t2_norm;
    t2[4] = 1.0 / t2_norm;
    t2[5] = -t2_mean[1] / t2_norm;
    t2[8] = 1.0;
}

void
apply_normalization  (math::Matrix<double, 3, 3> const& transform1,
    math::Matrix<double, 3, 3> const& transform2,
    Correspondences* correspondences)
{
    for (std::size_t i = 0; i < correspondences->size(); ++i)
    {
        Correspondence& c = correspondences->at(i);
        c.p1[0] = transform1[0] * c.p1[0] + transform1[2];
        c.p1[1] = transform1[4] * c.p1[1] + transform1[5];
        c.p2[0] = transform2[0] * c.p2[0] + transform2[2];
        c.p2[1] = transform2[4] * c.p2[1] + transform2[5];
    }
}

void
compute_normalization (Correspondences2D3D const& correspondences,
    math::Matrix<double, 3, 3>* transform_2d,
    math::Matrix<double, 4, 4>* transform_3d)
{
    math::Vector<double, 2> t2d_mean(0.0);
    math::Vector<double, 2> t2d_aabb_min(std::numeric_limits<double>::max());
    math::Vector<double, 2> t2d_aabb_max(-std::numeric_limits<double>::max());
    math::Vector<double, 3> t3d_mean(0.0);
    math::Vector<double, 3> t3d_aabb_min(std::numeric_limits<double>::max());
    math::Vector<double, 3> t3d_aabb_max(-std::numeric_limits<double>::max());

    for (std::size_t i = 0; i < correspondences.size(); ++i)
    {
        Correspondence2D3D const& c = correspondences[i];
        for (int j = 0; j < 2; ++j)
        {
            t2d_mean[j] += c.p2d[j];
            t2d_aabb_min[j] = std::min(t2d_aabb_min[j], c.p2d[j]);
            t2d_aabb_max[j] = std::max(t2d_aabb_max[j], c.p2d[j]);
        }
        for (int j = 0; j < 3; ++j)
        {
            t3d_mean[j] += c.p3d[j];
            t3d_aabb_min[j] = std::min(t3d_aabb_min[j], c.p3d[j]);
            t3d_aabb_max[j] = std::max(t3d_aabb_max[j], c.p3d[j]);
        }
    }

    t2d_mean /= static_cast<double>(correspondences.size());
    double t2d_norm = (t2d_aabb_max - t2d_aabb_min).maximum();
    math::Matrix<double, 3, 3>& t2d = *transform_2d;
    t2d.fill(0.0);
    t2d[0] = 1.0 / t2d_norm;
    t2d[4] = 1.0 / t2d_norm;
    t2d[8] = 1.0;
    t2d[2] = -t2d_mean[0] / t2d_norm;
    t2d[5] = -t2d_mean[1] / t2d_norm;

    t3d_mean /= static_cast<double>(correspondences.size());
    double t3d_norm = (t3d_aabb_max - t3d_aabb_min).maximum();
    math::Matrix<double, 4, 4>& t3d = *transform_3d;
    t3d.fill(0.0);
    t3d[0] = 1.0 / t3d_norm;
    t3d[5] = 1.0 / t3d_norm;
    t3d[10] = 1.0 / t3d_norm;
    t3d[15] = 1.0;
    t3d[3] = -t3d_mean[0] / t3d_norm;
    t3d[7] = -t3d_mean[1] / t3d_norm;
    t3d[11] = -t3d_mean[2] / t3d_norm;
}

void
apply_normalization (math::Matrix<double, 3, 3> const& transform_2d,
    math::Matrix<double, 4, 4> const& transform_3d,
    Correspondences2D3D* correspondences)
{
    for (std::size_t i = 0; i < correspondences->size(); ++i)
    {
        Correspondence2D3D& c = correspondences->at(i);
        c.p2d[0] = transform_2d[0] * c.p2d[0] + transform_2d[2];
        c.p2d[1] = transform_2d[4] * c.p2d[1] + transform_2d[5];
        c.p3d[0] = transform_3d[0] * c.p3d[0] + transform_3d[3];
        c.p3d[1] = transform_3d[5] * c.p3d[1] + transform_3d[7];
        c.p3d[2] = transform_3d[10] * c.p3d[2] + transform_3d[11];
    }
}

SFM_NAMESPACE_END
