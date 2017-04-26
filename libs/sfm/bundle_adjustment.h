/*
 * Copyright (C) 2015, Simon Fuhrmann, Fabian Langguth
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef SFM_BUNDLE_ADJUSTMENT_HEADER
#define SFM_BUNDLE_ADJUSTMENT_HEADER

#include <vector>

#include "util/logging.h"
#include "sfm/defines.h"
#include "sfm/ba_sparse_matrix.h"
#include "sfm/ba_dense_vector.h"
#include "sfm/ba_linear_solver.h"
#include "sfm/ba_types.h"

/*
 * A few notes.
 *
 * - PBA normalizes focal length and depth values before LM optimization,
 *   and denormalizes afterwards. Is this necessary with double?
 * - PBA exits the LM main loop if norm of -JF is small. Useful?
 * - The slowest part is computing the Schur complement because of matrix
 *   multiplications. How can this be improved?
 *
 * Actual TODOs.
 *
 * - Better preconditioner for conjugate gradient, i.e., use the 9x9 diagonal
 *   blocks of S instead of B. Requires method in matrix.
 * - Properly implement and test BA_POINTS mode.
 * - More accurate implementations for the Jacobian (currently approximated).
 * - Implement block_size = 9 in linear solver, no need for CG
 */

SFM_NAMESPACE_BEGIN
SFM_BA_NAMESPACE_BEGIN

/**
 * A simple bundle adjustment optimization implementation.
 * The algorithm requires good initial camera parameters and 3D points, as
 * well as observations of the 3D points in the cameras. The algorithm
 * then optimizes the 3D point positions and camera parameters in order to
 * minimize the reprojection errors, i.e., the distances from the point
 * projections to the observations.
 */
class BundleAdjustment
{
public:
    enum BAMode
    {
        BA_CAMERAS = 1,
        BA_POINTS = 2,
        BA_CAMERAS_AND_POINTS = 1 | 2
    };

    struct Options
    {
        Options (void);

        bool verbose_output;
        BAMode bundle_mode;
        bool fixed_intrinsics;
        //bool shared_intrinsics;
        int lm_max_iterations;
        int lm_min_iterations;
        double lm_delta_threshold;
        double lm_mse_threshold;
        LinearSolver::Options linear_opts;
    };

    struct Status
    {
        Status (void);

        double initial_mse;
        double final_mse;
        int num_lm_iterations;
        int num_lm_successful_iterations;
        int num_lm_unsuccessful_iterations;
        int num_cg_iterations;
        std::size_t runtime_ms;
    };

public:
    BundleAdjustment (Options const& options);

    void set_cameras (std::vector<Camera>* cameras);
    void set_points (std::vector<Point3D>* points);
    void set_observations (std::vector<Observation>* observations);

    Status optimize (void);
    void print_status (bool detailed = false) const;

private:
    typedef SparseMatrix<double> SparseMatrixType;
    typedef DenseVector<double> DenseVectorType;

private:
    void sanity_checks (void);
    void lm_optimize (void);

    /* Helper functions. */
    void compute_reprojection_errors (DenseVectorType* vector_f,
        DenseVectorType const* delta_x = nullptr);
    double compute_mse (DenseVectorType const& vector_f);
    void radial_distort (double* x, double* y, double const* dist);
    void rodrigues_to_matrix (double const* r, double* rot);

    /* Analytic Jacobian. */
    void analytic_jacobian (SparseMatrixType* jac_cam,
        SparseMatrixType* jac_points);
    void analytic_jacobian_entries (Camera const& cam, Point3D const& point,
        double* cam_x_ptr, double* cam_y_ptr,
        double* point_x_ptr, double* point_y_ptr);

    /* Update of camera/point parameters. */
    void update_parameters (DenseVectorType const& delta_x);
    void update_camera (Camera const& cam, double const* update, Camera* out);
    void update_point (Point3D const& pt, double const* update, Point3D* out);

private:
    Options opts;
    Status status;
    util::Logging log;
    std::vector<Camera>* cameras;
    std::vector<Point3D>* points;
    std::vector<Observation>* observations;
    int const num_cam_params;
};

/* ------------------------ Implementation ------------------------ */

inline
BundleAdjustment::Options::Options (void)
    : verbose_output(false)
    , bundle_mode(BA_CAMERAS_AND_POINTS)
    , lm_max_iterations(50)
    , lm_min_iterations(0)
    , lm_delta_threshold(1e-4)
    , lm_mse_threshold(1e-8)
{
}

inline
BundleAdjustment::Status::Status (void)
    : initial_mse(0.0)
    , final_mse(0.0)
    , num_lm_iterations(0)
    , num_lm_successful_iterations(0)
    , num_lm_unsuccessful_iterations(0)
    , num_cg_iterations(0)
{
}

inline
BundleAdjustment::BundleAdjustment (Options const& options)
    : opts(options)
    , log(options.verbose_output
        ? util::Logging::LOG_DEBUG
        : util::Logging::LOG_INFO)
    , cameras(nullptr)
    , points(nullptr)
    , observations(nullptr)
    , num_cam_params(options.fixed_intrinsics ? 6 : 9)
{
    this->opts.linear_opts.camera_block_dim = this->num_cam_params;
}

inline void
BundleAdjustment::set_cameras (std::vector<Camera>* cameras)
{
    this->cameras = cameras;
}

inline void
BundleAdjustment::set_points (std::vector<Point3D>* points)
{
    this->points = points;
}

inline void
BundleAdjustment::set_observations (std::vector<Observation>* points_2d)
{
    this->observations = points_2d;
}

SFM_BA_NAMESPACE_END
SFM_NAMESPACE_END

#endif /* SFM_BUNDLE_ADJUSTMENT_HEADER */
