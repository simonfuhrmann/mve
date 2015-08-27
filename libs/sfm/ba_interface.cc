/*
 * Copyright (C) 2015, Simon Fuhrmann, Fabian Langguth
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include "sfm/ba_interface.h"

#define ERROR this->log.error()
#define WARNING this->log.warning()
#define INFO this->log.info()
#define VERBOSE this->log.verbose()
#define DEBUG this->log.debug()

SFM_NAMESPACE_BEGIN
SFM_BA_NAMESPACE_BEGIN

BundleAdjustment::Status
BundleAdjustment::optimize (void)
{
    this->sanity_checks();
    this->status = Status();

    // TODO: Normalize data (focal length and depth, see PBA)?
    this->lm_optimize();
    // TODO: Denormalize data?

    return this->status;
}

void
BundleAdjustment::sanity_checks (void)
{
    /* Check for NULL arguments. */
    if (this->cameras == NULL)
        throw std::invalid_argument("No cameras given");
    if (this->points_3d == NULL)
        throw std::invalid_argument("No tracks given");
    if (this->points_2d == NULL)
        throw std::invalid_argument("No observations given");

    /* Check for valid focal lengths. */
    for (std::size_t i = 0; i < this->cameras->size(); ++i)
        if (this->cameras->at(i).focal_length <= 0.0)
            throw std::invalid_argument("Camera with invalid focal length");

    /* Check for valid IDs in the observations. */
    for (std::size_t i = 0; i < this->points_2d->size(); ++i)
    {
        Point2D const& p2d = this->points_2d->at(i);
        if (p2d.camera_id < 0
            || p2d.camera_id >= static_cast<int>(this->cameras->size()))
            throw std::invalid_argument("Observation with invalid camera ID");
        if (p2d.point3d_id < 0
            || p2d.point3d_id >= static_cast<int>(this->points_3d->size()))
            throw std::invalid_argument("Observation with invalid track ID");
    }
}

void
BundleAdjustment::lm_optimize (void)
{
    for (int lm_iter = 0; ; ++lm_iter)
    {
        /* Compute reprojection errors and MSE. */
        std::vector<double> F;
        this->compute_reprojection_errors(&F);
        double initial_mse = this->compute_mse(F);
        if (lm_iter == 0)
        {
            this->status.initial_mse = initial_mse;
            return; // TMP
        }

        // TODO: Do linear step

        // TODO: Compute new MSE
        double new_mse = 0.0;

        if (lm_iter + 1 < this->opts.lm_min_iterations)
            continue;

        if (lm_iter + 1 >= this->opts.lm_max_iterations)
        {
            INFO << "BA: Reached max LM iterations." << std::endl;
            break;
        }
        if (new_mse < this->opts.lm_mse_threshold)
        {
            INFO << "BA: Satisfied MSE threshold." << std::endl;
            break;
        }
        if (initial_mse - new_mse < this->opts.lm_delta_threshold)
        {
            INFO << "BA: Satisfied MSE delta threshold." << std::endl;
            break;
        }
    }
}

void
BundleAdjustment::compute_reprojection_errors (std::vector<double>* vector_f)
{
    vector_f->clear();
    vector_f->resize(this->points_2d->size() * 2);
    for (std::size_t i = 0, j = 0; i < this->points_2d->size(); ++i)
    {
        Point2D const& p2d = this->points_2d->at(i);
        Point3D const& p3d = this->points_3d->at(p2d.point3d_id);
        Camera const& cam = this->cameras->at(p2d.camera_id);

        /* Project point onto image plane. */
        double rp[] = { 0.0, 0.0, 0.0 };
        for (int d = 0; d < 3; ++d)
        {
            rp[0] += cam.rotation[0 + d] * p3d.pos[d];
            rp[1] += cam.rotation[3 + d] * p3d.pos[d];
            rp[2] += cam.rotation[6 + d] * p3d.pos[d];
        }
        rp[2] += cam.translation[2];
        rp[0] = (rp[0] + cam.translation[0]) * cam.focal_length / rp[2];
        rp[1] = (rp[1] + cam.translation[1]) * cam.focal_length / rp[2];

        /* Distort reprojections. */
        this->radial_distort(rp + 0, rp + 1, cam.distortion);

        /* Compute reprojection error. */
        vector_f->at(j++) = p2d.pos[0] - rp[0];
        vector_f->at(j++) = p2d.pos[1] - rp[1];
    }
}

double
BundleAdjustment::compute_mse (std::vector<double> const& vector_f)
{
    double mse = 0.0;
    for (std::size_t i = 0; i < vector_f.size(); ++i)
        mse += vector_f[i] * vector_f[i];
    return mse / static_cast<double>(vector_f.size() / 2);
}

void
BundleAdjustment::radial_distort (double* x, double* y, double const* dist)
{
    double const radius_pow2 = *x * *x + *y * *y;
    double const radius_pow4 = radius_pow2 * radius_pow2;
    double const factor = 1.0 + dist[0] * radius_pow2 + dist[1] * radius_pow4;
    *x *= factor;
    *y *= factor;
}

void
BundleAdjustment::print_options (void) const
{
    VERBOSE << "Bundle Adjustment Options:" << std::endl;
    VERBOSE << "  Verbose output: "
        << this->opts.verbose_output << std::endl;
}

void
BundleAdjustment::print_status (void) const
{
    VERBOSE << "Bundle Adjustment Status:" << std::endl;
    VERBOSE << "  Initial MSE: "
        << this->status.initial_mse << std::endl;
    VERBOSE << "  Final MSE: "
        << this->status.final_mse << std::endl;
    VERBOSE << "  LM iterations: "
        << this->status.num_lm_iterations << std::endl;
    VERBOSE << "  CG iterations: "
        << this->status.num_cg_iterations << std::endl;
}

SFM_BA_NAMESPACE_END
SFM_NAMESPACE_END
