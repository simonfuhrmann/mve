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

#include "sfm/ba_linear_solver.h"

SFM_NAMESPACE_BEGIN
SFM_BA_NAMESPACE_BEGIN

void
LinearSolver::solve (MatrixType const& jacobian,
    VectorType const& values, VectorType* delta_x)
{
    std::size_t jac_rows = values.size();
    std::size_t jac_cols = jacobian.size() / jac_rows;

    typedef Eigen::SparseMatrix<double> SparseMatrixType;
    typedef Eigen::Map<Eigen::VectorXd const> MappedVectorType;
    typedef Eigen::Matrix<double, Eigen::Dynamic, 1> DenseVectorType;
    typedef std::vector<Eigen::Triplet<double> > TripletsType;

    TripletsType triplets;
    for (std::size_t i = 0; i < jacobian.size(); ++i)
    {
        if (jacobian[i] == 0.0)
            continue;
        std::size_t col = i % jac_cols;
        std::size_t row = i / jac_cols;
        triplets.push_back(Eigen::Triplet<double>(row, col, jacobian[i]));
    }

    SparseMatrixType J(jac_rows, jac_cols);
    J.setFromTriplets(triplets.begin(), triplets.end());
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

    std::cout << "BA: CG reported: ";
    switch (cg.info())
    {
        case Eigen::Success:
            std::cout << "Computation was successful." << std::endl; break;
        case Eigen::NumericalIssue:
            std::cout << "Provided data did not satisfy the prerequisites." << std::endl; break;
        case Eigen::NoConvergence:
            std::cout << "Iterative procedure did not converge." << std::endl; break;
        case Eigen::InvalidInput:
            std::cout << "The inputs are invalid." << std::endl; break;
        default:
            std::cout << "Unknown info." << std::endl; break;
    }

    if (delta_x->size() != jac_cols)
    {
        delta_x->clear();
        delta_x->resize(jac_cols, 0.0);
    }

    //std::cout << "DX = [";
    for (std::size_t i = 0; i < jac_cols; ++i)
    {
        delta_x->at(i) = x[i];
        //std::cout << delta_x->at(i) << " ";
    }
    //std::cout << "]" << std::endl;
}

SFM_BA_NAMESPACE_END
SFM_NAMESPACE_END
