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
    };

    struct Status
    {
        Status (void);

        double predicted_error_decrease;
        int num_cg_iterations;
        bool cg_success;
    };

    typedef SparseMatrix<double> SparseMatrixType;
    typedef DenseVector<double> DenseVectorType;

public:
    LinearSolver (Options const& options);

    /* Conjugate Gradient on Schur-complement. */
    Status solve_schur (SparseMatrixType const& jac_cams,
        SparseMatrixType const& jac_points,
        DenseVectorType const& values, DenseVectorType* delta_x);

    /* Conjugate Gradient on H = J^T * J. */
    //Status solve (SparseMatrixType const& jac_cams,
    //    SparseMatrixType const& jac_points,
    //    DenseVectorType const& values, DenseVectorType* delta_x);

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
    , cg_success(false)
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

