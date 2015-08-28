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
    /* Levenberg-Marquard main loop. */
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

        /* Compute Jacobian. */
        std::vector<double> J;
        this->compute_jacobian(&J);

        // TODO: Do linear step

        /* Compute MSE after linear step. */
        double new_mse = this->compute_mse(F);

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

//#pragma omp parallel for
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
        rp[0] = (rp[0] + cam.translation[0]) / rp[2];
        rp[1] = (rp[1] + cam.translation[1]) / rp[2];

        /* Distort reprojections. */
        this->radial_distort(rp + 0, rp + 1, cam.distortion);

        /* Compute reprojection error. */
        vector_f->at(j++) = p2d.pos[0] - rp[0] * cam.focal_length;
        vector_f->at(j++) = p2d.pos[1] - rp[1] * cam.focal_length;
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
BundleAdjustment::compute_jacobian (std::vector<double>* matrix_j)
{
    std::size_t const camera_off = this->cameras->size() * 9;
    std::size_t const matrix_rows = this->points_2d->size() * 2;
    std::size_t const matrix_cols = camera_off + this->points_3d->size() * 3;

    matrix_j->clear();
    matrix_j->resize(matrix_rows * matrix_cols, 0.0);
    double* base_ptr = &matrix_j->at(0);

//#pragma omp parallel for
    for (std::size_t i = 0, j = 0; i < this->points_2d->size(); ++i, j += 2)
    {
        Point2D const& p2d = this->points_2d->at(i);
        Point3D const& p3d = this->points_3d->at(p2d.point3d_id);
        Camera const& cam = this->cameras->at(p2d.camera_id);
        double* row_x_ptr = base_ptr + matrix_cols * (j + 0);
        double* row_y_ptr = base_ptr + matrix_cols * (j + 1);
        double* cam_x_ptr = row_x_ptr + p2d.camera_id * 9;
        double* cam_y_ptr = row_y_ptr + p2d.camera_id * 9;
        double* point_x_ptr = row_x_ptr + camera_off + p2d.point3d_id * 3;
        double* point_y_ptr = row_y_ptr + camera_off + p2d.point3d_id * 3;
        this->compute_jacobian_entries(cam, p3d,
            cam_x_ptr, cam_y_ptr, point_x_ptr, point_y_ptr);
    }
}

void
BundleAdjustment::compute_jacobian_entries (Camera const& cam,
    Point3D const& point,
    double* cam_x_ptr, double* cam_y_ptr,
    double* point_x_ptr, double* point_y_ptr)
{
    /* Aliases. */
    double const* rot = cam.rotation;
    double const* trans = cam.translation;
    double const* dist = cam.distortion;
    double const* p3d = point.pos;

    /* Temporary values. */
    double const px = rot[0] * p3d[0] + rot[1] * p3d[1] + rot[2] * p3d[2];
    double const py = rot[3] * p3d[0] + rot[4] * p3d[1] + rot[5] * p3d[2];
    double const pz = rot[6] * p3d[0] + rot[7] * p3d[1] + rot[8] * p3d[2];
    double const ix = px / pz;
    double const iy = py / pz;
    double const radius2 = ix * ix + iy * iy;
    double const radius4 = radius2 * radius2;
    double const rd_factor = 1.0 + dist[0] * radius2 + dist[1] * radius4;

    /*
     * Compute camera derivatives. One camera block consists of:
     * element 0: derivative of focal length f
     * element 1-2: derivative of distortion parameters k0, k1
     * element 3-5: derivative of translation t0, t1, t2
     * element 6-8: derivative of rotation r0, r1, r2
     */
    cam_x_ptr[0] = ix * rd_factor;
    cam_y_ptr[0] = iy * rd_factor;

    /*
     * Compute point derivatives.
     * element 0: Derivative in x-direction of 3D point.
     * element 1: Derivative in y-direction of 3D point.
     * element 2: Derivative in z-direction of 3D point.
     */
    //point_x_ptr[0] =
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
