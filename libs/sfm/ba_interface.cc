/*
 * Copyright (C) 2015, Simon Fuhrmann, Fabian Langguth
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include "math/matrix_tools.h"
#include "sfm/ba_interface.h"

#define ERROR this->log.error()
#define WARNING this->log.warning()
#define INFO this->log.info()
#define VERBOSE this->log.verbose()
#define DEBUG this->log.debug()

SFM_NAMESPACE_BEGIN
SFM_BA_NAMESPACE_BEGIN

/*
 * A few notes and TODOs.
 *
 * - PBA normalizes focal length and depth values before LM optimization,
 *   and denormalizes afterwards. Is this necessary with double?
 */

BundleAdjustment::Status
BundleAdjustment::optimize (void)
{
    this->sanity_checks();
    this->status = Status();
    this->lm_optimize();
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
            this->status.initial_mse = initial_mse;

        /* Compute Jacobian. */
        std::vector<double> J;
        this->analytic_jacobian(&J);

        return; // TMP

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
    if (vector_f->size() != this->points_2d->size() * 2)
    {
        vector_f->clear();
        vector_f->resize(this->points_2d->size() * 2);
    }

//#pragma omp parallel for
    for (std::size_t i = 0; i < this->points_2d->size(); ++i)
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
        vector_f->at(i * 2 + 0) = p2d.pos[0] - rp[0] * cam.focal_length;
        vector_f->at(i * 2 + 1) = p2d.pos[1] - rp[1] * cam.focal_length;
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
BundleAdjustment::analytic_jacobian (std::vector<double>* matrix_j)
{
    std::size_t const camera_off = this->cameras->size() * 9;
    std::size_t const matrix_rows = this->points_2d->size() * 2;
    std::size_t const matrix_cols = camera_off + this->points_3d->size() * 3;

    matrix_j->clear();
    matrix_j->resize(matrix_rows * matrix_cols, 0.0);
    double* base_ptr = &matrix_j->at(0);

//#pragma omp parallel for
    for (std::size_t i = 0; i < this->points_2d->size(); ++i)
    {
        Point2D const& p2d = this->points_2d->at(i);
        Point3D const& p3d = this->points_3d->at(p2d.point3d_id);
        Camera const& cam = this->cameras->at(p2d.camera_id);
        double* row_x_ptr = base_ptr + matrix_cols * (i * 2 + 0);
        double* row_y_ptr = base_ptr + matrix_cols * (i * 2 + 1);
        double* cam_x_ptr = row_x_ptr + p2d.camera_id * 9;
        double* cam_y_ptr = row_y_ptr + p2d.camera_id * 9;
        double* point_x_ptr = row_x_ptr + camera_off + p2d.point3d_id * 3;
        double* point_y_ptr = row_y_ptr + camera_off + p2d.point3d_id * 3;
        this->analytic_jacobian_entries(cam, p3d,
            cam_x_ptr, cam_y_ptr, point_x_ptr, point_y_ptr);
    }

    /* Print jacobian. */
    std::cout << "[";
    for (std::size_t r = 0; r < matrix_rows; ++r)
    {
        for (std::size_t c = 0; c < matrix_cols; ++c)
            std::cout << " " << matrix_j->at(r * matrix_cols + c);
        std::cout << ";" << std::endl;
    }
    std::cout << "]" << std::endl;

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
    point_x_ptr[0] = fz * rd_factor * p3d[0] * (r[0] - r[6] * ix);
    point_x_ptr[1] = fz * rd_factor * p3d[1] * (r[1] - r[7] * ix);
    point_x_ptr[2] = fz * rd_factor * p3d[2] * (r[2] - r[8] * ix);

    point_y_ptr[0] = fz * rd_factor * p3d[0] * (r[3] - r[6] * iy);
    point_y_ptr[1] = fz * rd_factor * p3d[1] * (r[4] - r[7] * iy);
    point_y_ptr[2] = fz * rd_factor * p3d[2] * (r[5] - r[8] * iy);
#elif JACOBIAN_APPROX_PBA

#else

#endif
}

void
BundleAdjustment::numeric_jacobian (std::vector<double>* matrix_j)
{
    std::size_t const num_cam_params = this->cameras->size() * 9;
    std::size_t const num_point_params = this->points_3d->size() * 3;
    std::size_t const rows = this->points_2d->size() * 2;
    std::size_t const cols = num_cam_params + num_point_params;

    matrix_j->clear();
    matrix_j->resize(rows * cols, 0.0);

    /* Numeric differentiation for cameras. */
    for (std::size_t i = 0; i < this->cameras->size(); ++i)
    {
        std::size_t col = i * 9;
        Camera& camera = this->cameras->at(i);
        this->numeric_jacobian_member(&camera.focal_length,
            1e-6, matrix_j, col + 0, cols);
        this->numeric_jacobian_member(camera.distortion + 0,
            1e-6, matrix_j, col + 1, cols);
        this->numeric_jacobian_member(camera.distortion + 1,
            1e-6, matrix_j, col + 2, cols);
        this->numeric_jacobian_member(camera.translation + 0,
            1e-6, matrix_j, col + 3, cols);
        this->numeric_jacobian_member(camera.translation + 1,
            1e-6, matrix_j, col + 4, cols);
        this->numeric_jacobian_member(camera.translation + 2,
            1e-6, matrix_j, col + 5, cols);
        this->numeric_jacobian_rotation(camera.rotation,
            1e-3, matrix_j, 0, 6, cols);
        this->numeric_jacobian_rotation(camera.rotation,
            1e-3, matrix_j, 1, 7, cols);
        this->numeric_jacobian_rotation(camera.rotation,
            1e-3, matrix_j, 2, 8, cols);
    }

    /* Numeric differentiation for points. */
    for (std::size_t i = 0; i < this->points_3d->size(); ++i)
    {
        std::size_t col = num_cam_params + i * 3;
        Point3D& point = this->points_3d->at(i);
        this->numeric_jacobian_member(point.pos + 0,
            1e-6, matrix_j, col + 0, cols);
        this->numeric_jacobian_member(point.pos + 1,
            1e-6, matrix_j, col + 1, cols);
        this->numeric_jacobian_member(point.pos + 2,
            1e-6, matrix_j, col + 2, cols);
    }
}

void
BundleAdjustment::numeric_jacobian_member (double* field, double eps,
    std::vector<double>* matrix_j, std::size_t col, std::size_t cols)
{
    std::vector<double> f1, f2;
    double backup = *field;
    *field = backup - eps;
    this->compute_reprojection_errors(&f1);
    *field = backup + eps;
    this->compute_reprojection_errors(&f2);
    *field = backup;

    for (std::size_t i = 0, j = col; i < matrix_j->size(); i += 1, j += cols)
        matrix_j->at(j) = (f2[i] - f1[i]) / (2.0 * eps);
}

void
BundleAdjustment::numeric_jacobian_rotation (double* rot, double eps,
    std::vector<double>* matrix_j,
    std::size_t axis, std::size_t col, std::size_t cols)
{
    double backup[9];
    std::copy(rot, rot + 9, backup);
    double delta_rot_m[9];
    double delta_rot_r[] = { 0.0, 0.0, 0.0 };
    std::vector<double> f1, f2;

    delta_rot_r[axis] = -eps;
    this->rodrigues_to_matrix(delta_rot_r, delta_rot_m);
    math::matrix_multiply(backup, 3, 3, delta_rot_m, 3, rot);
    this->compute_reprojection_errors(&f1);

    delta_rot_r[axis] = eps;
    this->rodrigues_to_matrix(delta_rot_r, delta_rot_m);
    math::matrix_multiply(backup, 3, 3, delta_rot_m, 3, rot);
    this->compute_reprojection_errors(&f2);

    std::copy(backup, backup + 9, rot);

    for (std::size_t i = 0, j = col; i < matrix_j->size(); i += 1, j += cols)
        matrix_j->at(j) = (f2[i] - f1[i]) / (2.0 * eps);
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
