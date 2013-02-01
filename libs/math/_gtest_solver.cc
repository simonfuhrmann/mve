/*
 * Test cases for solver.
 * Written by Jens Ackermann.
 */

#include <gtest/gtest.h>

#include "math/jacobisolver.h"

TEST(SolverTest, JacobiSolverTest)
{
    using namespace math;
    JacobiSolverParams<float> params;
    params.maxIter = 100;
    params.minResidual = 0.0f;

    Matrix3f A;
    A(0,0) = -2.0f; A(0,1) = 0.0f; A(0,2) = 0.0f;
    A(1,0) = 4.0f; A(1,1) = -3.0f; A(1,2) = -1.0f;
    A(2,0) = 0.0f; A(2,1) = -4.0f; A(2,2) = 4.0f;

    Vec3f rhs(2.0f, 4.0f, 16.0f);

    JacobiSolver<float, 3> solver(A, params);
    Vec3f solution = solver.solve(rhs, Vec3f(0.0f));

    //std::cout << "iterations: " << solver.getInfo().maxIter << ", residual norm: " << solver.getInfo().minResidual << std::endl;
    //std::cout << "Solution: " << solution << std::endl;
    Vec3f exactSolution(-1.0f, -3.0f, 1.0f);
    EXPECT_TRUE(exactSolution.is_similar(solution, 0.0f));
}
