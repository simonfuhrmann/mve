/*
 * Copyright (C) 2015, Simon Fuhrmann, Fabian Langguth
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <eigen3/Eigen/Core>
#include <eigen3/Eigen/Sparse>
#include <eigen3/Eigen/QR>

#include <iostream>

#include "math/matrix.h"
#include "math/matrix_tools.h"
#include "sfm/ba_linear_solver.h"

SFM_NAMESPACE_BEGIN
SFM_BA_NAMESPACE_BEGIN


bool
LinearSolver::solve_schur (MatrixType const& jac_cams,
    MatrixType const& jac_points,
    VectorType const& values, VectorType* delta_x)
{
    /*
     * Jacobian J = [ Jc Jp ] with Jc camera block, Jp point block.
     * Hessian H = [ B E; E^T C ] = J^T J = [ Jc^T; Jp^T ] * [ Jc Jp ]
     * with  B = Jc^T * Jc  and  E = Jc^T * Jp  and  C = Jp^T Jp
     */

    typedef std::vector<Eigen::Triplet<double> > TripletsType;
    typedef Eigen::SparseMatrix<double> SparseMatrixType;
    typedef Eigen::Matrix<double, Eigen::Dynamic, 1> DenseVectorType;

    std::size_t jac_rows = values.size();
    std::size_t jac_cam_cols = jac_cams.size() / jac_rows;
    std::size_t jac_point_cols = jac_points.size() / jac_rows;
    std::size_t jac_cols = jac_cam_cols + jac_point_cols;

    /* Assemble sparse matrices for the camera Jacobian. */
    SparseMatrixType Jc(jac_rows, jac_cam_cols);
    {
        TripletsType triplets;
        for (std::size_t i = 0; i < jac_cams.size(); ++i)
        {
            if (jac_cams[i] == 0.0)
                continue;
            std::size_t col = i % jac_cam_cols;
            std::size_t row = i / jac_cam_cols;
            triplets.emplace_back(row, col, jac_cams[i]);
        }
        Jc.setFromTriplets(triplets.begin(), triplets.end());
    }

    /* Assemble sparse matrices for the point Jacobian. */
    SparseMatrixType Jp(jac_rows, jac_point_cols);
    {
        TripletsType triplets;
        for (std::size_t i = 0; i < jac_points.size(); ++i)
        {
            if (jac_points[i] == 0.0)
                continue;
            std::size_t col = i % jac_point_cols;
            std::size_t row = i / jac_point_cols;
            triplets.emplace_back(row, col, jac_points[i]);
        }
        Jp.setFromTriplets(triplets.begin(), triplets.end());
    }

    /* Assemble two values vectors. */
    DenseVectorType v(jac_cam_cols);
    for (std::size_t i = 0; i < jac_cam_cols; ++i)
        v[i] = values[i];
    DenseVectorType w(jac_point_cols);
    for (std::size_t i = 0; i < jac_point_cols; ++i)
        w[i] = values[jac_cam_cols + i];

    /* Compute the blocks of the Hessian. */
    SparseMatrixType B = Jc.transpose() * Jc;
    SparseMatrixType E = Jc.transpose() * Jp;
    SparseMatrixType C = Jp.transpose() * Jp;

    /* Add regularization to C and B. */
    if (this->opts.trust_region_radius != 0.0)
    {
        TripletsType triplets;
        for (std::size_t i = 0; i < C.cols(); ++i)
            triplets.emplace_back(i, i, C.diagonal()[i] / this->opts.trust_region_radius);
        SparseMatrixType C_diag(C.cols(), C.cols());
        C_diag.setFromTriplets(triplets.begin(), triplets.end());
        C = C + C_diag;
    }

    if (this->opts.trust_region_radius != 0.0)
    {
        TripletsType triplets;
        for (std::size_t i = 0; i < B.cols(); ++i)
            triplets.emplace_back(i, i, B.diagonal()[i] / this->opts.trust_region_radius);
        SparseMatrixType B_diag(B.cols(), B.cols());
        B_diag.setFromTriplets(triplets.begin(), triplets.end());
        B = B + B_diag;
    }

    /* Invert C matrix. */
    SparseMatrixType C_inv(jac_point_cols, jac_point_cols);
    {
        std::vector<math::Matrix3d> Cm(jac_point_cols / 3, math::Matrix3d(0.0));
        for (int i = 0; i < C.outerSize(); ++i)
            for (SparseMatrixType::InnerIterator it(C, i); it; ++it)
                Cm[it.row() / 3][(it.row() % 3) * 3 + (it.col() % 3)] = it.value();
        for (std::size_t i = 0; i < Cm.size(); ++i)
            Cm[i] = math::matrix_inverse(Cm[i]);

        TripletsType triplets;
        for (std::size_t i = 0; i < Cm.size(); ++i)
            for (std::size_t j = 0; j < 9; ++j)
                triplets.emplace_back(i * 3 + j / 3, i * 3 + j % 3, Cm[i][j]);
        C_inv.setFromTriplets(triplets.begin(), triplets.end());
    }

    /* Compute the Schur complement matrix S. */
    SparseMatrixType S = B - E * C_inv * E.transpose();
    DenseVectorType rhs = v - E * (C_inv * w);

    /* Solve linear system. */
    Eigen::ConjugateGradient<SparseMatrixType> cg;
    cg.compute(S);
    DenseVectorType delta_y = cg.solve(rhs);

    switch (cg.info())
    {
        case Eigen::Success:
            std::cout << "BA: CG converged." << std::endl;
            break;
        case Eigen::NumericalIssue:
            std::cout << "BA: CG failed (data prerequisites)" << std::endl;
            return false;
        case Eigen::NoConvergence:
            std::cout << "BA: CG failed (not converged)" << std::endl;
            return false;
        case Eigen::InvalidInput:
            std::cout << "BA: CG failed (invalid input)" << std::endl;
            return false;
        default:
            std::cout << "BA: CG failed (unknown error)" << std::endl;
            return false;
    }

    /* Substitute back to obtain delta z. */
    DenseVectorType delta_z = C_inv * (w - E.transpose() * delta_y);

    /* Fill output vector. */
    if (delta_x->size() != jac_cols)
    {
        delta_x->clear();
        delta_x->resize(jac_cols, 0.0);
    }
    for (std::size_t i = 0; i < jac_cam_cols; ++i)
        delta_x->at(i) = delta_y[i];
    for (std::size_t i = 0; i < jac_point_cols; ++i)
        delta_x->at(jac_cam_cols + i) = delta_z[i];

    return true;
}

bool
LinearSolver::solve (MatrixType const& jac_cams, MatrixType const& jac_points,
    VectorType const& values, VectorType* delta_x)
{
    std::size_t jac_rows = values.size();
    std::size_t jac_cam_cols = jac_cams.size() / jac_rows;
    std::size_t jac_point_cols = jac_points.size() / jac_rows;
    std::size_t jac_cols = jac_cam_cols + jac_point_cols;

    typedef Eigen::SparseMatrix<double> SparseMatrixType;
    typedef Eigen::Map<Eigen::VectorXd const> MappedVectorType;
    typedef Eigen::Matrix<double, Eigen::Dynamic, 1> DenseVectorType;
    typedef std::vector<Eigen::Triplet<double> > TripletsType;

    SparseMatrixType J(jac_rows, jac_cols);
    {
        TripletsType triplets;
        for (std::size_t i = 0; i < jac_cams.size(); ++i)
        {
            if (jac_cams[i] == 0.0)
                continue;
            std::size_t col = i % jac_cam_cols;
            std::size_t row = i / jac_cam_cols;
            triplets.push_back(Eigen::Triplet<double>(row, col, jac_cams[i]));
        }
        for (std::size_t i = 0; i < jac_points.size(); ++i)
        {
            if (jac_points[i] == 0.0)
                continue;
            std::size_t col = i % jac_point_cols;
            std::size_t row = i / jac_point_cols;
            triplets.push_back(Eigen::Triplet<double>(row, jac_cam_cols + col, jac_points[i]));
        }

        J.setFromTriplets(triplets.begin(), triplets.end());
    }

    //std::cout << J << std::endl;

    MappedVectorType F(values.data(), values.size());
    SparseMatrixType H = J.transpose() * J;
    DenseVectorType g = -J.transpose() * F;

    if (this->opts.trust_region_radius != 0.0)
    {
        TripletsType H_diag_triplets;
        for (std::size_t i = 0; i < jac_cols; ++i)
            H_diag_triplets.push_back(Eigen::Triplet<double>(i, i,
                H.diagonal()[i] / this->opts.trust_region_radius));
        SparseMatrixType H_diag(jac_cols, jac_cols);
        H_diag.setFromTriplets(H_diag_triplets.begin(), H_diag_triplets.end());
        H = H + H_diag;
    }

    Eigen::ConjugateGradient<SparseMatrixType> cg;
    cg.compute(H);
    DenseVectorType x = cg.solve(g);

    if (cg.info() == Eigen::Success)
    {
        if (delta_x->size() != jac_cols)
        {
            delta_x->clear();
            delta_x->resize(jac_cols, 0.0);
        }

        for (std::size_t i = 0; i < jac_cols; ++i)
            delta_x->at(i) = x[i];
    }

    switch (cg.info())
    {
        case Eigen::Success:
            std::cout << "BA: CG converged." << std::endl;
            return true;
        case Eigen::NumericalIssue:
            std::cout << "BA: CG failed (data prerequisites)" << std::endl;
            return false;
        case Eigen::NoConvergence:
            std::cout << "BA: CG failed (not converged)" << std::endl;
            return false;
        case Eigen::InvalidInput:
            std::cout << "BA: CG failed (invalid input)" << std::endl;
            return false;
        default:
            std::cout << "BA: CG failed (unknown error)" << std::endl;
            return false;
    }
}

SFM_BA_NAMESPACE_END
SFM_NAMESPACE_END