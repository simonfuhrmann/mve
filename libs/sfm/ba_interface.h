#ifndef SFM_BA_INTERFACE_HEADER
#define SFM_BA_INTERFACE_HEADER

#include <algorithm>
#include <vector>

#include "util/logging.h"
#include "sfm/defines.h"
#include "sfm/ba_linear_solver.h"

SFM_NAMESPACE_BEGIN
SFM_BA_NAMESPACE_BEGIN

struct Camera
{
    Camera (void);

    double focal_length;
    double distortion[2];
    double translation[3];
    double rotation[9];
    bool is_constant;
};

struct Point3D
{
    double pos[3];
};

struct Point2D
{
    double pos[2];
    int camera_id;
    int point3d_id;
};

class BundleAdjustment
{
public:
    enum BAMode
    {
        BA_CAMERAS_AND_POINTS,
        BA_CAMERAS_ONLY,
        BA_POINTS_ONLY
    };

    struct Options
    {
        Options (void);

        bool verbose_output;
        //BAMode bundle_mode;
        //bool fixed_intrinsics;
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
    };

public:
    BundleAdjustment (Options const& options);

    void set_cameras (std::vector<Camera>* cameras);
    void set_points_3d (std::vector<Point3D>* points_3d);
    void set_points_2d (std::vector<Point2D>* points_2d);

    Status optimize (void);
    void print_options (void) const;
    void print_status (void) const;

private:
    typedef std::vector<double> SparseMatrix;
    typedef std::vector<double> DenseVector;

private:
    void sanity_checks (void);
    void lm_optimize (void);

    /* Helper functions. */
    void compute_reprojection_errors (DenseVector* vector_f,
        DenseVector const* delta_x = NULL);
    double compute_mse (DenseVector const& vector_f);
    void radial_distort (double* x, double* y, double const* dist);
    void rodrigues_to_matrix (double const* r, double* rot);
    void print_jacobian (SparseMatrix const& matrix_j,
        char const* prefix) const;

    /* Analytic Jacobian. */
    void analytic_jacobian (SparseMatrix* matrix_j);
    void analytic_jacobian_entries (Camera const& cam, Point3D const& point,
        double* cam_x_ptr, double* cam_y_ptr,
        double* point_x_ptr, double* point_y_ptr);

    /* Numeric Jacobian. */
    void numeric_jacobian (SparseMatrix* matrix_j);
    void numeric_jacobian_member (double* field, double eps,
        SparseMatrix* matrix_j, std::size_t col, std::size_t cols);
    void numeric_jacobian_rotation (double* rot, double eps,
        SparseMatrix* matrix_j,
        std::size_t axis, std::size_t col, std::size_t cols);

    /* */
    void update_parameters (DenseVector const& delta_x);
    void update_camera (Camera const& cam, double const* update, Camera* out);
    void update_point (Point3D const& pt, double const* update, Point3D* out);

private:
    Options opts;
    Status status;
    util::Logging log;
    std::vector<Camera>* cameras;
    std::vector<Point3D>* points_3d;
    std::vector<Point2D>* points_2d;
};

/* ------------------------ Implementation ------------------------ */

inline
Camera::Camera (void)
    : focal_length(0.0)
    , is_constant(false)
{
    std::fill(this->distortion, this->distortion + 2, 0.0);
    std::fill(this->translation, this->translation + 3, 0.0);
    std::fill(this->rotation, this->rotation + 9, 0.0);
}

inline
BundleAdjustment::Options::Options (void)
    : verbose_output(false)
    //, bundle_mode(BA_CAMERAS_AND_POINTS)
    , lm_max_iterations(100)
    , lm_min_iterations(0)
    , lm_delta_threshold(1e-12)
    , lm_mse_threshold(1e-16)
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
        ? util::Logging::DEBUG
        : util::Logging::INFO)
    , cameras(NULL)
    , points_3d(NULL)
    , points_2d(NULL)
{
}

inline void
BundleAdjustment::set_cameras (std::vector<Camera>* cameras)
{
    this->cameras = cameras;
}

inline void
BundleAdjustment::set_points_3d (std::vector<Point3D>* points_3d)
{
    this->points_3d = points_3d;
}

inline void
BundleAdjustment::set_points_2d (std::vector<Point2D>* points_2d)
{
    this->points_2d = points_2d;
}

SFM_BA_NAMESPACE_END
SFM_NAMESPACE_END

#endif /* SFM_BA_INTERFACE_HEADER */

