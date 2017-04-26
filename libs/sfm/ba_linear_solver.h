/*
 * Copyright (C) 2015, Simon Fuhrmann, Fabian Langguth
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef SFM_BA_LINEAR_SOLVER_HEADER
#define SFM_BA_LINEAR_SOLVER_HEADER

#include <vector>

#include "sfm/defines.h"
#include "sfm/ba_sparse_matrix.h"
#include "sfm/ba_dense_vector.h"

SFM_NAMESPACE_BEGIN
SFM_BA_NAMESPACE_BEGIN

class LinearSolver
{
public:
    struct Options
    {
        Options (void);

        double trust_region_radius;
        int cg_max_iterations;
        int camera_block_dim;
    };

    struct Status
    {
        Status (void);

        double predicted_error_decrease;
        int num_cg_iterations;
        bool success;
    };

    typedef SparseMatrix<double> SparseMatrixType;
    typedef DenseVector<double> DenseVectorType;

public:
    LinearSolver (Options const& options);

    /**
     * Solve the system J^T J x = -J^T f based on the bundle adjustment mode.
     * If the Jacobian for cameras is empty, only points are optimized.
     * If the Jacobian for points is empty, only cameras are optimized.
     * If both, Jacobian for cams and points is given, the Schur complement
     * trick is used to solve the linear system.
     */
    Status solve (SparseMatrixType const& jac_cams,
        SparseMatrixType const& jac_points,
        DenseVectorType const& vector_f,
        DenseVectorType* delta_x);

private:
    /**
     * Conjugate Gradient on Schur-complement by exploiting the block
     * structure of H = J^T * J.
     */
    Status solve_schur (SparseMatrixType const& jac_cams,
        SparseMatrixType const& jac_points,
        DenseVectorType const& values,
        DenseVectorType* delta_x);

    /**
     * J is the Jacobian of the problem. If H = J^T * J has a block diagonal
     * structure (e.g. 'motion only' or 'structure only' problems in BA),
     * block_size can be used to directly invert H. If block_size is 0
     * the diagonal of H is used as a preconditioner and the linear system
     * is solved via conjugate gradient.
     */
    Status solve (SparseMatrixType const& J,
        DenseVectorType const& vector_f,
        DenseVectorType* delta_x,
        std::size_t block_size = 0);

private:
    Options opts;
};

/* ------------------------ Implementation ------------------------ */

inline
LinearSolver::Options::Options (void)
    : trust_region_radius(1.0)
    , cg_max_iterations(1000)
{
}

inline
LinearSolver::Status::Status (void)
    : predicted_error_decrease(0.0)
    , num_cg_iterations(0)
    , success(false)
{
}

inline
LinearSolver::LinearSolver (Options const& options)
    : opts(options)
{
}

SFM_BA_NAMESPACE_END
SFM_NAMESPACE_END

#endif // SFM_BA_LINEAR_SOLVER_HEADER

