// Test cases for the conjugate gradient solver
// Written by Fabian Langguth

#include <gtest/gtest.h>

#include "sfm/ba_conjugate_gradient.h"

TEST(ConjugateGradientTest, CGInvalidInputTest)
{
    typedef sfm::ba::ConjugateGradient<double> CGSolver;
    typedef sfm::ba::SparseMatrix<double> SparseMatrix;
    typedef sfm::ba::DenseVector<double> DenseVector;

    CGSolver::Options opts;
    opts.max_iterations = 4;
    CGSolver solver(opts);

    SparseMatrix A(4, 4);
    DenseVector b(3);
    DenseVector x;

    CGSolver::Status status = solver.solve(A, b, &x);

    EXPECT_EQ(CGSolver::CG_INVALID_INPUT, status.info);
}

TEST(ConjugateGradientTest, CGSolverTest)
{
    typedef sfm::ba::ConjugateGradient<double> CGSolver;
    typedef sfm::ba::SparseMatrix<double> SparseMatrix;
    typedef sfm::ba::DenseVector<double> DenseVector;

    CGSolver::Options opts;
    opts.max_iterations = 4;
    opts.tolerance = 1e-40;
    CGSolver solver(opts);

    SparseMatrix A(4, 4);
    SparseMatrix::Triplets triplets;
    triplets.emplace_back(0, 0, 1.0);
    triplets.emplace_back(1, 1, 2.0);
    triplets.emplace_back(2, 2, 3.0);
    triplets.emplace_back(3, 3, 4.0);
    A.set_from_triplets(triplets);

    DenseVector b(4);
    b[0] = 1;
    b[1] = 1;
    b[2] = 1;
    b[3] = 1;

    DenseVector x;
    CGSolver::Status status = solver.solve(A, b, &x);

    EXPECT_EQ(CGSolver::CG_MAX_ITERATIONS, status.info);
    EXPECT_NEAR(x[0], 1.0, 1e-14);
    EXPECT_NEAR(x[1], 1.0 / 2.0, 1e-14);
    EXPECT_NEAR(x[2], 1.0 / 3.0, 1e-14);
    EXPECT_NEAR(x[3], 1.0 / 4.0, 1e-14);
}

TEST(ConjugateGradientTest, CGSolverExplicitFunctorTest)
{
    typedef sfm::ba::ConjugateGradient<double> CGSolver;
    typedef sfm::ba::SparseMatrix<double> SparseMatrix;
    typedef sfm::ba::DenseVector<double> DenseVector;

    CGSolver::Options opts;
    opts.max_iterations = 4;
    opts.tolerance = 1e-40;
    CGSolver solver(opts);

    SparseMatrix A(4,4);
    SparseMatrix::Triplets triplets;
    triplets.emplace_back(0, 0, 1.0);
    triplets.emplace_back(1, 1, 2.0);
    triplets.emplace_back(2, 2, 3.0);
    triplets.emplace_back(3, 3, 4.0);
    A.set_from_triplets(triplets);

    DenseVector b(4);
    b[0] = 1;
    b[1] = 1;
    b[2] = 1;
    b[3] = 1;

    DenseVector x;
    CGSolver::Status status = solver.solve(
        sfm::ba::CGBasicMatrixFunctor<double>(A), b, &x);

    EXPECT_EQ(CGSolver::CG_MAX_ITERATIONS, status.info);
    EXPECT_NEAR(x[0], 1.0, 1e-14);
    EXPECT_NEAR(x[1], 1.0 / 2.0, 1e-14);
    EXPECT_NEAR(x[2], 1.0 / 3.0, 1e-14);
    EXPECT_NEAR(x[3], 1.0 / 4.0, 1e-14);
}

TEST(ConjugateGradientTest, PreconditionedCGSolverExactTest)
{
    typedef sfm::ba::ConjugateGradient<double> CGSolver;
    typedef sfm::ba::SparseMatrix<double> SparseMatrix;
    typedef sfm::ba::DenseVector<double> DenseVector;

    CGSolver::Options opts;
    opts.max_iterations = 4;
    opts.tolerance = 1e-40;
    CGSolver solver(opts);

    SparseMatrix A(4, 4);
    SparseMatrix::Triplets tripletsA;
    tripletsA.emplace_back(0, 0, 1.0);
    tripletsA.emplace_back(1, 1, 2.0);
    tripletsA.emplace_back(2, 2, 3.0);
    tripletsA.emplace_back(3, 3, 4.0);
    A.set_from_triplets(tripletsA);

    SparseMatrix P(4, 4);
    SparseMatrix::Triplets tripletsP;
    tripletsP.emplace_back(0, 0, 1.0 / 1.0);
    tripletsP.emplace_back(1, 1, 1.0 / 2.0);
    tripletsP.emplace_back(2, 2, 1.0 / 3.0);
    tripletsP.emplace_back(3, 3, 1.0 / 4.0);
    P.set_from_triplets(tripletsP);

    DenseVector b(4);
    b[0] = 1;
    b[1] = 1;
    b[2] = 1;
    b[3] = 1;

    DenseVector x;
    CGSolver::Status status = solver.solve(A, b, &x, &P);

    EXPECT_EQ(0, status.num_iterations);
    EXPECT_EQ(CGSolver::CG_CONVERGENCE, status.info);
    EXPECT_NEAR(x[0], 1.0, 1e-14);
    EXPECT_NEAR(x[1], 1.0 / 2.0, 1e-14);
    EXPECT_NEAR(x[2], 1.0 / 3.0, 1e-14);
    EXPECT_NEAR(x[3], 1.0 / 4.0, 1e-14);
}

TEST(ConjugateGradientTest, PreconditionedCGSolverApproximateTest)
{
    typedef sfm::ba::ConjugateGradient<double> CGSolver;
    typedef sfm::ba::SparseMatrix<double> SparseMatrix;
    typedef sfm::ba::DenseVector<double> DenseVector;

    CGSolver::Options opts;
    opts.max_iterations = 4;
    opts.tolerance = 1e-40;
    CGSolver solver(opts);

    SparseMatrix A(4, 4);
    SparseMatrix::Triplets tripletsA;
    tripletsA.emplace_back(0, 0, 1.0);
    tripletsA.emplace_back(1, 1, 2.0);
    tripletsA.emplace_back(2, 2, 3.0);
    tripletsA.emplace_back(3, 3, 4.0);
    A.set_from_triplets(tripletsA);

    SparseMatrix P(4, 4);
    SparseMatrix::Triplets tripletsP;
    tripletsP.emplace_back(0, 0, 1.0 / 1.0);
    tripletsP.emplace_back(1, 1, 1.0 / 1.0);
    tripletsP.emplace_back(2, 2, 1.0 / 2.0);
    tripletsP.emplace_back(3, 3, 1.0 / 3.0);
    P.set_from_triplets(tripletsP);

    DenseVector b(4);
    b[0] = 1;
    b[1] = 1;
    b[2] = 1;
    b[3] = 1;

    DenseVector x;
    solver.solve(A, b, &x, &P);

    EXPECT_NEAR(x[0], 1.0, 1e-14);
    EXPECT_NEAR(x[1], 1.0 / 2.0, 1e-14);
    EXPECT_NEAR(x[2], 1.0 / 3.0, 1e-14);
    EXPECT_NEAR(x[3], 1.0 / 4.0, 1e-14);
}
