/*
 * Copyright (C) 2015, Simon Fuhrmann, Fabian Langguth
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <iostream>

#include "util/timer.h"
#include "math/matrix.h"
#include "math/matrix_tools.h"
#include "sfm/ba_linear_solver.h"
#include "sfm/ba_conjugate_gradient.h"

SFM_NAMESPACE_BEGIN
SFM_BA_NAMESPACE_BEGIN

namespace
{
    void
    invert_block_matrix_3x3_inplace (SparseMatrix<double>* M)
    {
        for (double* iter = M->begin(); iter != M->end(); )
        {
            double* iter_backup = iter;
            math::Matrix<double, 3, 3> rot;
            for (int i = 0; i < 9; ++i)
                rot[i] = *(iter++);
            rot = math::matrix_inverse(rot);
            iter = iter_backup;
            for (int i = 0; i < 9; ++i)
                *(iter++) = rot[i];
        }
    }
}

LinearSolver::Status
LinearSolver::solve_schur (SparseMatrixType const& jac_cams,
    SparseMatrixType const& jac_points,
    DenseVectorType const& values, DenseVectorType* delta_x)
{
    /*
     * Jacobian J = [ Jc Jp ] with Jc camera block, Jp point block.
     * Hessian H = [ B E; E^T C ] = J^T J = [ Jc^T; Jp^T ] * [ Jc Jp ]
     * with  B = Jc^T * Jc  and  E = Jc^T * Jp  and  C = Jp^T Jp
     */
    DenseVectorType const& F = values;
    SparseMatrixType const& Jc = jac_cams;
    SparseMatrixType const& Jp = jac_points;
    SparseMatrixType JcT = Jc.transpose();
    SparseMatrixType JpT = Jp.transpose();

    /* Compute the blocks of the Hessian. */
    SparseMatrixType B = JcT.multiply(Jc);
    SparseMatrixType E = JcT.multiply(Jp);
    SparseMatrixType C = JpT.multiply(Jp);  // Most time-consuming product.

    /* Assemble two values vectors. */
    DenseVectorType v = JcT.multiply(F);
    DenseVectorType w = JpT.multiply(F);
    v.negate_self();
    w.negate_self();

    /* Add regularization to C and B. */
    C.mult_diagonal(1.0 + 1.0 / this->opts.trust_region_radius);
    B.mult_diagonal(1.0 + 1.0 / this->opts.trust_region_radius);

    /* Invert C matrix. */
    invert_block_matrix_3x3_inplace(&C);

    /* Compute the Schur complement matrix S. */
    SparseMatrixType ET = E.transpose();
    SparseMatrixType S = B.subtract(E.multiply(C).multiply(ET));
    DenseVectorType rhs = v.subtract(E.multiply(C.multiply(w)));

    /* Compute pre-conditioner for linear system. */
    SparseMatrixType precond = S.diagonal_matrix();
    precond.cwise_invert();

    /* Solve linear system. */
    DenseVectorType delta_y(Jc.num_cols());
    typedef sfm::ba::ConjugateGradient<double> CGSolver;
    CGSolver::Options cg_opts;
    cg_opts.max_iterations = this->opts.cg_max_iterations;
    cg_opts.tolerance = 1e-20;
    CGSolver solver(cg_opts);
    CGSolver::Status cg_status;
    cg_status = solver.solve(S, rhs, &delta_y, &precond);

    Status status;
    status.num_cg_iterations = cg_status.num_iterations;
    switch (cg_status.info)
    {
        case CGSolver::CG_CONVERGENCE:
            status.cg_success = true;
            break;
        case CGSolver::CG_MAX_ITERATIONS:
            status.cg_success = true;
            break;
        case CGSolver::CG_INVALID_INPUT:
            std::cout << "BA: CG failed (invalid input)" << std::endl;
            status.cg_success = false;
            return status;
        default:
            break;
    }

    /* Substitute back to obtain delta z. */
    DenseVectorType delta_z = C.multiply(w.subtract(ET.multiply(delta_y)));

    /* Fill output vector. */
    std::size_t const jac_cam_cols = Jc.num_cols();
    std::size_t const jac_point_cols = Jp.num_cols();
    std::size_t const jac_cols = jac_cam_cols + jac_point_cols;

    if (delta_x->size() != jac_cols)
        delta_x->resize(jac_cols, 0.0);
    for (std::size_t i = 0; i < jac_cam_cols; ++i)
        delta_x->at(i) = delta_y[i];
    for (std::size_t i = 0; i < jac_point_cols; ++i)
        delta_x->at(jac_cam_cols + i) = delta_z[i];

    /* Compute predicted error decrease */
    status.predicted_error_decrease = 0.0;
    status.predicted_error_decrease += delta_y.dot(
        (delta_y.multiply(1.0 / this->opts.trust_region_radius).add(v)));
    status.predicted_error_decrease += delta_z.dot(
        (delta_z.multiply(1.0 / this->opts.trust_region_radius).add(w)));

    return status;
}

LinearSolver::Status
LinearSolver::solve (SparseMatrixType const& J, DenseVectorType const& values,
    DenseVectorType* delta_x, std::size_t block_size)
{
    DenseVectorType const& F = values;
    SparseMatrixType Jt = J.transpose();

    SparseMatrixType H = Jt.multiply(J);
    DenseVectorType g = Jt.multiply(F);
    g.negate_self();

    H.mult_diagonal(1.0 + 1.0 / this->opts.trust_region_radius);

    Status status;

    if (block_size == 0)
    {
        /* Use simple preconditioned CG */
        SparseMatrixType precond = H.diagonal_matrix();
        precond.cwise_invert();

        typedef sfm::ba::ConjugateGradient<double> CGSolver;
        CGSolver::Options cg_opts;
        cg_opts.max_iterations = this->opts.cg_max_iterations;
        cg_opts.tolerance = 1e-20;
        CGSolver solver(cg_opts);
        CGSolver::Status cg_status;
        cg_status = solver.solve(H, g, delta_x, &precond);

        status.num_cg_iterations = cg_status.num_iterations;

        switch (cg_status.info)
        {
            case CGSolver::CG_CONVERGENCE:
                status.cg_success = true;
                break;
            case CGSolver::CG_MAX_ITERATIONS:
                status.cg_success = true;
                break;
            case CGSolver::CG_INVALID_INPUT:
                std::cout << "BA: CG failed (invalid input)" << std::endl;
                status.cg_success = false;
                return status;
            default:
                break;
        }
    }
    else if (block_size == 3)
    {
        /* Invert blocks of H directly */
        invert_block_matrix_3x3_inplace(&H);
        *delta_x = H.multiply(g);
    }
    else
    {
        throw std::invalid_argument("Invalid block_size in linear solver.");
    }

    status.predicted_error_decrease = delta_x->dot(
        delta_x->multiply(1.0 / this->opts.trust_region_radius).add(g));

    return status;
}

SFM_BA_NAMESPACE_END
SFM_NAMESPACE_END
