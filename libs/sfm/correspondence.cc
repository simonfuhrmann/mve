#include "correspondence.h"

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
    t1[0] = 1.0 / t1_norm; t1[1] = 0.0;           t1[2] = -t1_mean[0] / t1_norm;
    t1[3] = 0.0;           t1[4] = 1.0 / t1_norm; t1[5] = -t1_mean[1] / t1_norm;
    t1[6] = 0.0;           t1[7] = 0.0;           t1[8] = 1.0;

    t2_mean /= static_cast<double>(correspondences.size());
    double t2_norm = (t2_aabb_max - t2_aabb_min).maximum();
    math::Matrix<double, 3, 3>& t2 = *transform2;
    t2[0] = 1.0 / t2_norm; t2[1] = 0.0;           t2[2] = -t2_mean[0] / t2_norm;
    t2[3] = 0.0;           t2[4] = 1.0 / t2_norm; t2[5] = -t2_mean[1] / t2_norm;
    t2[6] = 0.0;           t2[7] = 0.0;           t2[8] = 1.0;
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

SFM_NAMESPACE_END
