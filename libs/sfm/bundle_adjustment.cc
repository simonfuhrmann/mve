/*
 * Copyright (C) 2015, Simon Fuhrmann, Fabian Langguth
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <iostream>
#include <iomanip>

#include "math/matrix_tools.h"
#include "util/timer.h"
#include "sfm/ba_sparse_matrix.h"
#include "sfm/ba_dense_vector.h"
#include "sfm/bundle_adjustment.h"

#define LOG_E this->log.error()
#define LOG_W this->log.warning()
#define LOG_I this->log.info()
#define LOG_V this->log.verbose()
#define LOG_D this->log.debug()

#define TRUST_REGION_RADIUS_INIT (1000)
#define TRUST_REGION_RADIUS_DECREMENT (1.0 / 2.0)

SFM_NAMESPACE_BEGIN
SFM_BA_NAMESPACE_BEGIN

/*
 * A few notes and TODOs.
 *
 * - PBA normalizes focal length and depth values before LM optimization,
 *   and denormalizes afterwards. Is this necessary with double?
 * - PBA exits the LM main loop if norm of -JF is small. Useful?
 */

BundleAdjustment::Status
BundleAdjustment::optimize (void)
{
    util::WallTimer timer;
    this->sanity_checks();
    this->status = Status();
    this->lm_optimize();
    this->status.runtime_ms = timer.get_elapsed();
    return this->status;
}

void
BundleAdjustment::sanity_checks (void)
{
    /* Check for null arguments. */
    if (this->cameras == nullptr)
        throw std::invalid_argument("No cameras given");
    if (this->points_3d == nullptr)
        throw std::invalid_argument("No tracks given");
    if (this->points_2d == nullptr)
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
    /* Setup linear solver. */
    LinearSolver::Options pcg_opts;
    pcg_opts = this->opts.linear_opts;
    pcg_opts.trust_region_radius = TRUST_REGION_RADIUS_INIT;

    /* Compute reprojection error for the first time. */
    DenseVectorType F, F_new;
    this->compute_reprojection_errors(&F);
    double current_mse = this->compute_mse(F);
    this->status.initial_mse = current_mse;
    this->status.final_mse = current_mse;

    /* Levenberg-Marquard main loop. */
    for (int lm_iter = 0; ; ++lm_iter)
    {
        if (lm_iter + 1 > this->opts.lm_min_iterations
            && (current_mse < this->opts.lm_mse_threshold))
        {
            LOG_I << "BA: Satisfied MSE threshold." << std::endl;
            break;
        }

        /* Compute Jacobian. */
        SparseMatrixType Jc, Jp;
        switch (this->opts.bundle_mode)
        {
            case BA_CAMERAS_AND_POINTS:
                this->analytic_jacobian(&Jc, &Jp);
                break;
            case BA_CAMERAS:
                this->analytic_jacobian(&Jc, nullptr);
                break;
            case BA_POINTS:
                this->analytic_jacobian(nullptr, &Jp);
                break;
            default:
                throw std::runtime_error("");
        }

        /* Perform linear step. */
        DenseVectorType delta_x;
        LinearSolver pcg(pcg_opts);
        LinearSolver::Status cg_status = pcg.solve(Jc, Jp, F, &delta_x);

        /* Update reprojection errors and MSE after linear step. */
        double new_mse, delta_mse;
        if (cg_status.success)
        {
            this->compute_reprojection_errors(&F_new, &delta_x);
            new_mse = this->compute_mse(F_new);
            delta_mse = current_mse - new_mse;
            this->status.num_cg_iterations += cg_status.num_cg_iterations;
        }
        else
        {
            new_mse = current_mse;
            delta_mse = 0.0;
        }
        bool successful_iteration = delta_mse > 0.0;

        /*
         * Apply delta to parameters after successful step.
         * Adjust the trust region to increase/decrease regulariztion.
         */
        if (successful_iteration)
        {
            LOG_V << "BA: #" << std::setw(2) << lm_iter << " success"
                << ", MSE " << std::setw(11) << current_mse
                << " -> " << std::setw(11) << new_mse
                << ", CG " << std::setw(3) << cg_status.num_cg_iterations
                << ", TRR " << pcg_opts.trust_region_radius
                << std::endl;

            this->status.num_lm_iterations += 1;
            this->status.num_lm_successful_iterations += 1;
            this->update_parameters(delta_x);
            std::swap(F, F_new);
            current_mse = new_mse;

            /* Compute trust region update. */
            double const gain_ratio = delta_mse * (F.size() / 2)
                / cg_status.predicted_error_decrease;
            double const trust_region_update = 1.0 / std::max(1.0 / 3.0,
                (1.0 - MATH_POW3(2.0 * gain_ratio - 1.0)));
            pcg_opts.trust_region_radius *= trust_region_update;
        }
        else
        {
            LOG_V << "BA: #" << std::setw(2) << lm_iter << " failure"
                << ", MSE " << std::setw(11) << current_mse
                << ",    " << std::setw(11) << " "
                << " CG " << std::setw(3) << cg_status.num_cg_iterations
                << ", TRR " << pcg_opts.trust_region_radius
                << std::endl;

            this->status.num_lm_iterations += 1;
            this->status.num_lm_unsuccessful_iterations += 1;
            pcg_opts.trust_region_radius *= TRUST_REGION_RADIUS_DECREMENT;
        }

        /* Check termination of LM iterations. */
        if (lm_iter + 1 < this->opts.lm_min_iterations)
            continue;

        if (lm_iter + 1 >= this->opts.lm_max_iterations)
        {
            LOG_I << "BA: Reached max LM iterations." << std::endl;
            break;
        }
        if (successful_iteration && delta_mse < this->opts.lm_delta_threshold)
        {
            LOG_I << "BA: Satisfied MSE delta threshold." << std::endl;
            break;
        }
    }

    this->status.final_mse = current_mse;
}

void
BundleAdjustment::compute_reprojection_errors (DenseVectorType* vector_f,
    DenseVectorType const* delta_x)
{
    if (vector_f->size() != this->points_2d->size() * 2)
        vector_f->resize(this->points_2d->size() * 2);

//#pragma omp parallel for
    for (std::size_t i = 0; i < this->points_2d->size(); ++i)
    {
        Point2D const& p2d = this->points_2d->at(i);
        Point3D const& p3d = this->points_3d->at(p2d.point3d_id);
        Camera const& cam = this->cameras->at(p2d.camera_id);

        double const* flen = &cam.focal_length;
        double const* dist = cam.distortion;
        double const* rot = cam.rotation;
        double const* trans = cam.translation;
        double const* point = p3d.pos;

        Point3D new_point;
        Camera new_camera;
        if (delta_x != nullptr)
        {
            std::size_t cam_id = p2d.camera_id * 9;
            std::size_t pt_id = p2d.point3d_id * 3;

            if (this->opts.bundle_mode & BA_CAMERAS)
            {
                this->update_camera(cam, delta_x->data() + cam_id, &new_camera);
                flen = &new_camera.focal_length;
                dist = new_camera.distortion;
                rot = new_camera.rotation;
                trans = new_camera.translation;
                pt_id += this->cameras->size() * 9;
            }

            if (this->opts.bundle_mode & BA_POINTS)
            {
                this->update_point(p3d, delta_x->data() + pt_id, &new_point);
                point = new_point.pos;
            }
        }

        /* Project point onto image plane. */
        double rp[] = { 0.0, 0.0, 0.0 };
        for (int d = 0; d < 3; ++d)
        {
            rp[0] += rot[0 + d] * point[d];
            rp[1] += rot[3 + d] * point[d];
            rp[2] += rot[6 + d] * point[d];
        }
        rp[2] = (rp[2] + trans[2]);
        rp[0] = (rp[0] + trans[0]) / rp[2];
        rp[1] = (rp[1] + trans[1]) / rp[2];

        /* Distort reprojections. */
        this->radial_distort(rp + 0, rp + 1, dist);

        /* Compute reprojection error. */
        vector_f->at(i * 2 + 0) = rp[0] * (*flen) - p2d.pos[0];
        vector_f->at(i * 2 + 1) = rp[1] * (*flen) - p2d.pos[1];
    }
}

double
BundleAdjustment::compute_mse (DenseVectorType const& vector_f)
{
    double mse = 0.0;
    for (std::size_t i = 0; i < vector_f.size(); ++i)
        mse += vector_f[i] * vector_f[i];
    return mse / static_cast<double>(vector_f.size() / 2);
}

void
BundleAdjustment::radial_distort (double* x, double* y, double const* dist)
{
    double const radius2 = *x * *x + *y * *y;
    double const factor = 1.0 + radius2 * (dist[0] + dist[1] * radius2);
    *x *= factor;
    *y *= factor;
}

void
BundleAdjustment::rodrigues_to_matrix (double const* r, double* m)
{
    /* Obtain angle from vector length. */
    double a = std::sqrt(r[0] * r[0] + r[1] * r[1] + r[2] * r[2]);
    /* Precompute sine and cosine terms. */
    double ct = (a == 0.0) ? 0.5f : (1.0f - std::cos(a)) / (2.0 * a);
    double st = (a == 0.0) ? 1.0 : std::sin(a) / a;
    /* R = I + st * K + ct * K^2 (with cross product matrix K). */
    m[0] = 1.0 - (r[1] * r[1] + r[2] * r[2]) * ct;
    m[1] = r[0] * r[1] * ct - r[2] * st;
    m[2] = r[2] * r[0] * ct + r[1] * st;
    m[3] = r[0] * r[1] * ct + r[2] * st;
    m[4] = 1.0f - (r[2] * r[2] + r[0] * r[0]) * ct;
    m[5] = r[1] * r[2] * ct - r[0] * st;
    m[6] = r[2] * r[0] * ct - r[1] * st;
    m[7] = r[1] * r[2] * ct + r[0] * st;
    m[8] = 1.0 - (r[0] * r[0] + r[1] * r[1]) * ct;
}

void
BundleAdjustment::analytic_jacobian (SparseMatrixType* jac_cam,
    SparseMatrixType* jac_points)
{
    std::size_t const camera_cols = this->cameras->size() * 9;
    std::size_t const point_cols = this->points_3d->size() * 3;
    std::size_t const jacobi_rows = this->points_2d->size() * 2;

    SparseMatrixType::Triplets cam_triplets, point_triplets;
    if (jac_cam != nullptr)
        cam_triplets.reserve(this->points_2d->size() * 9 * 2);
    if (jac_points != nullptr)
        point_triplets.reserve(this->points_2d->size() * 3 * 2);

    double cam_x_ptr[9], cam_y_ptr[9], point_x_ptr[3], point_y_ptr[3];
    for (std::size_t i = 0; i < this->points_2d->size(); ++i)
    {
        Point2D const& p2d = this->points_2d->at(i);
        Point3D const& p3d = this->points_3d->at(p2d.point3d_id);
        Camera const& cam = this->cameras->at(p2d.camera_id);
        this->analytic_jacobian_entries(cam, p3d,
            cam_x_ptr, cam_y_ptr, point_x_ptr, point_y_ptr);

        std::size_t row_x = i * 2, row_y = row_x + 1;
        std::size_t cam_col = p2d.camera_id * 9;
        std::size_t point_col = p2d.point3d_id * 3;
        for (int j = 0; jac_cam != nullptr && j < 9; ++j)
        {
            cam_triplets.emplace_back(row_x, cam_col + j, cam_x_ptr[j]);
            cam_triplets.emplace_back(row_y, cam_col + j, cam_y_ptr[j]);
        }
        for (int j = 0; jac_points != nullptr && j < 3; ++j)
        {
            point_triplets.emplace_back(row_x, point_col + j, point_x_ptr[j]);
            point_triplets.emplace_back(row_y, point_col + j, point_y_ptr[j]);
        }
    }

    if (jac_cam != nullptr)
    {
        jac_cam->allocate(jacobi_rows, camera_cols);
        jac_cam->set_from_triplets(cam_triplets);
    }
    if (jac_points != nullptr)
    {
        jac_points->allocate(jacobi_rows, point_cols);
        jac_points->set_from_triplets(point_triplets);
    }
}

void
BundleAdjustment::analytic_jacobian_entries (Camera const& cam,
    Point3D const& point,
    double* cam_x_ptr, double* cam_y_ptr,
    double* point_x_ptr, double* point_y_ptr)
{
    /*
     * This function computes the Jacobian entries for the given camera and
     * 3D point pair that leads to one observation.
     *
     * The camera block 'cam_x_ptr' and 'cam_y_ptr' is:
     * - ID 0: Derivative of focal length f
     * - ID 1-2: Derivative of distortion parameters k0, k1
     * - ID 3-5: Derivative of translation t0, t1, t2
     * - ID 6-8: Derivative of rotation r0, r1, r2
     *
     * The 3D point block 'point_x_ptr' and 'point_y_ptr' is:
     * - ID 0-2: Derivative in x, y, and z direction.
     *
     * The function that leads to the observation is given as follows:
     *
     *   Px = f * D(ix,iy) * ix  (image observation x coordinate)
     *   Py = f * D(ix,iy) * iy  (image observation y coordinate)
     *
     * with the following definitions:
     *
     *   x = R0 * X + t0  (homogeneous projection)
     *   y = R1 * X + t1  (homogeneous projection)
     *   z = R2 * X + t2  (homogeneous projection)
     *   ix = x / z  (central projection)
     *   iy = y / z  (central projection)
     *   D(ix, iy) = 1 + k0 (ix^2 + iy^2) + k1 (ix^2 + iy^2)^2  (distortion)
     *
     * The derivatives for intrinsics (f, k0, k1) are easy to compute exactly.
     * The derivatives for extrinsics (r, t) and point coordinates Xx, Xy, Xz,
     * are a bit of a pain to compute.
     */

    /* Aliases. */
    double const* r = cam.rotation;
    double const* t = cam.translation;
    double const* k = cam.distortion;
    double const* p3d = point.pos;

    /* Temporary values. */
    double const rx = r[0] * p3d[0] + r[1] * p3d[1] + r[2] * p3d[2];
    double const ry = r[3] * p3d[0] + r[4] * p3d[1] + r[5] * p3d[2];
    double const rz = r[6] * p3d[0] + r[7] * p3d[1] + r[8] * p3d[2];
    double const px = rx + t[0];
    double const py = ry + t[1];
    double const pz = rz + t[2];
    double const ix = px / pz;
    double const iy = py / pz;
    double const fz = cam.focal_length / pz;
    double const radius2 = ix * ix + iy * iy;
    double const rd_factor = 1.0 + (k[0] + k[1] * radius2) * radius2;

    /* The intrinsics are easy to compute exactly. */
    cam_x_ptr[0] = ix * rd_factor;
    cam_x_ptr[1] = cam.focal_length * ix * radius2;
    cam_x_ptr[2] = cam.focal_length * ix * radius2 * radius2;

    cam_y_ptr[0] = iy * rd_factor;
    cam_y_ptr[1] = cam.focal_length * iy * radius2;
    cam_y_ptr[2] = cam.focal_length * iy * radius2 * radius2;

#define JACOBIAN_APPROX_CONST_RD 1
#define JACOBIAN_APPROX_PBA 0
#if JACOBIAN_APPROX_CONST_RD
    /*
     * Compute approximations of the Jacobian entries for the extrinsics
     * by assuming the distortion coefficent D(ix, iy) is constant.
     */
    cam_x_ptr[3] = fz * rd_factor;
    cam_x_ptr[4] = 0.0;
    cam_x_ptr[5] = -fz * rd_factor * ix;
    cam_x_ptr[6] = -fz * rd_factor * ry * ix;
    cam_x_ptr[7] = fz * rd_factor * (rz + rx * ix);
    cam_x_ptr[8] = -fz * rd_factor * ry;

    cam_y_ptr[3] = 0.0;
    cam_y_ptr[4] = fz * rd_factor;
    cam_y_ptr[5] = -fz * rd_factor * iy;
    cam_y_ptr[6] = -fz * rd_factor * (rz + ry * iy);
    cam_y_ptr[7] = fz * rd_factor * rx * iy;
    cam_y_ptr[8] = fz * rd_factor * rx;

    /*
     * Compute point derivatives in x, y, and z.
     */
    point_x_ptr[0] = fz * rd_factor * (r[0] - r[6] * ix);
    point_x_ptr[1] = fz * rd_factor * (r[1] - r[7] * ix);
    point_x_ptr[2] = fz * rd_factor * (r[2] - r[8] * ix);

    point_y_ptr[0] = fz * rd_factor * (r[3] - r[6] * iy);
    point_y_ptr[1] = fz * rd_factor * (r[4] - r[7] * iy);
    point_y_ptr[2] = fz * rd_factor * (r[5] - r[8] * iy);
#elif JACOBIAN_APPROX_PBA
    /* Computation of Jacobian approximation with one distortion argument. */
    // TODO
#else
    /* Computation of the full Jacobian. */
    // TODO
#endif
}

void
BundleAdjustment::update_parameters (DenseVectorType const& delta_x)
{
    /* Update cameras. */
    std::size_t num_camera_params = 0;
    if (this->opts.bundle_mode & BA_CAMERAS)
    {
        for (std::size_t i = 0; i < this->cameras->size(); ++i)
            this->update_camera(this->cameras->at(i),
                delta_x.data() + 9 * i, &this->cameras->at(i));
        num_camera_params = this->cameras->size() * 9;
    }

    /* Update points. */
    if (this->opts.bundle_mode & BA_POINTS)
    {

        for (std::size_t i = 0; i < this->points_3d->size(); ++i)
            this->update_point(this->points_3d->at(i),
                delta_x.data() + num_camera_params + i * 3,
                &this->points_3d->at(i));
    }
}

void
BundleAdjustment::update_camera (Camera const& cam,
    double const* update, Camera* out)
{
    out->focal_length = cam.focal_length + update[0];
    out->distortion[0] = cam.distortion[0] + update[1];
    out->distortion[1] = cam.distortion[1] + update[2];
    out->translation[0] = cam.translation[0] + update[3];
    out->translation[1] = cam.translation[1] + update[4];
    out->translation[2] = cam.translation[2] + update[5];

    double rot_orig[9];
    std::copy(cam.rotation, cam.rotation + 9, rot_orig);
    double rot_update[9];
    this->rodrigues_to_matrix(update + 6, rot_update);
    math::matrix_multiply(rot_update, 3, 3, rot_orig, 3, out->rotation);
}

void
BundleAdjustment::update_point (Point3D const& pt,
    double const* update, Point3D* out)
{
    out->pos[0] = pt.pos[0] + update[0];
    out->pos[1] = pt.pos[1] + update[1];
    out->pos[2] = pt.pos[2] + update[2];
}

void
BundleAdjustment::print_status (bool detailed) const
{
    if (!detailed)
    {
        std::cout << "BA: MSE " << this->status.initial_mse
            << " -> " << this->status.final_mse << ", "
            << this->status.num_lm_iterations << " LM iters, "
            << this->status.num_cg_iterations << " CG iters, "
            << this->status.runtime_ms << "ms."
            << std::endl;
        return;
    }

    std::cout << "Bundle Adjustment Status:" << std::endl;
    std::cout << "  Initial MSE: "
        << this->status.initial_mse << std::endl;
    std::cout << "  Final MSE: "
        << this->status.final_mse << std::endl;
    std::cout << "  LM iterations: "
        << this->status.num_lm_iterations << " ("
        << this->status.num_lm_successful_iterations << " successful, "
        << this->status.num_lm_unsuccessful_iterations << " unsuccessful)"
        << std::endl;
    std::cout << "  CG iterations: "
        << this->status.num_cg_iterations << std::endl;
}

SFM_BA_NAMESPACE_END
SFM_NAMESPACE_END
