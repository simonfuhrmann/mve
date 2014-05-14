/*
 * A simple Jacobi method to solve a system of linear equations.
 * Written by Jens Ackermann.
 */

#ifndef MATH_JACOBISOLVER_HEADER
#define MATH_JACOBISOLVER_HEADER

#include <climits>
#include <cmath>

#include "math/defines.h"
#include "math/matrix.h"
#include "math/vector.h"

MATH_NAMESPACE_BEGIN

/**
 * This structs combines the convergence criteria/info
 * for the Jacobi solver.
 */
template <typename T>
struct JacobiSolverParams
{
    int maxIter;
    T minResidual;

    JacobiSolverParams (void);
};


/**
 * The Jacobi method solves a linear system Ax=b iteratively.
 * The matrix A is decomposed into the diagonal D and S := A-D.
 * We can then reformulate:
 *      b = Ax = (D + S) * x    ==>     x = D^{-1} * (b - S*x)
 * This gives an iterative scheme x^{k*1} = D^{-1} * (b - S*x^{k}).
 *
 * Convergence can be controlled by specifying a maximal
 * number of iterations and/or a minimal value for the norm
 * of the residual ||b - Ax||^2.
 *
 * Convergence is only guaranteed for strictly diagonally
 * dominant matrices.
 * TODO: Implement check for diagonal dominance
 */
template <typename T, int N>
class JacobiSolver
{
public:
    /* ------------------------ Constructors ---------------------- */

    /** Default constructor. */
    JacobiSolver (void);

    /** Constructor from a matrix using default parameters. */
    JacobiSolver (Matrix<T,N,N> const&);

    /** Constructor from a matrix and a set of parameters. */
    JacobiSolver (Matrix<T,N,N> const&, JacobiSolverParams<T> const&);

    /* ------------------------ Member Access --------------------- */

    /** Sets the matrix used for subsequent computations. */
    void setMatrix (Matrix<T,N,N> const&);

    /** Retrieves the matrix that is currently used. */
    Matrix<T,N,N> getMatrix (void) const;

    /** Sets the parameters (i.e. convergence conditions)
     * used for subsequent computations.
     */
    void setParams (JacobiSolverParams<T> const&);

    /** Retrieves the current parameters. */
    JacobiSolverParams<T> getParams (void) const;

    /** Returns convergence info for the last run. */
    JacobiSolverParams<T> getInfo (void) const;

    /* --------------------------- Solver ----------------------------- */

    /**
     * Start iterative solver for a certain right hand side.
     * If you omit the initialisation vector, it is set to all zeros.
     */
    Vector<T,N> solve (Vector<T,N> const&, Vector<T,N> const&);

private:
    Vector<T,N> diagonal; // TODO: switch to invers of diagonal?
    Matrix<T,N,N> nonDiagMatrix;

    JacobiSolverParams<T> params;
    JacobiSolverParams<T> resultInfo;

    /**
     * Common initialisation (e.g of parameters) shared between constructors.
     */
    void init(void);
};

MATH_NAMESPACE_END


/* ------------------------ Implementation ------------------------ */

MATH_NAMESPACE_BEGIN

template <typename T>
inline
JacobiSolverParams<T>::JacobiSolverParams (void)
{
    this->maxIter = 100;
    this->minResidual = T(0);
}

template <typename T, int N>
inline
JacobiSolver<T,N>::JacobiSolver (void)
{
    init();
}

template <typename T, int N>
inline
JacobiSolver<T,N>::JacobiSolver (Matrix<T,N,N> const& mat)
{
    init();
    setMatrix(mat);
}

template <typename T, int N>
inline
JacobiSolver<T,N>::JacobiSolver (Matrix<T,N,N> const& mat,
    JacobiSolverParams<T> const& params)
{
    init();
    setMatrix(mat);
    setParams(params);
}

template <typename T, int N>
inline void
JacobiSolver<T,N>::init(void)
{
    this->resultInfo = this->params;
}

template <typename T, int N>
inline void
JacobiSolver<T,N>::setMatrix (Matrix<T,N,N> const& mat)
{
    this->nonDiagMatrix = mat;
    for (std::size_t i=0; i < N; i++) {
        this->diagonal[i] = mat[i * (N+1)];
        this->nonDiagMatrix[i * (N+1)] = T(0);
    }
}

template <typename T, int N>
inline Matrix<T,N,N>
JacobiSolver<T,N>::getMatrix (void) const
{
    Matrix<T,N,N> result = this->nonDiagMatrix;
    for (std::size_t i=0; i < N; i++) {
        result[i * (N+1)] = this->diagonal[i];
    }
    return result;
}

template <typename T, int N>
inline void
JacobiSolver<T,N>::setParams (JacobiSolverParams<T> const& params)
{
    this->params = params;
}

template <typename T, int N>
inline JacobiSolverParams<T>
JacobiSolver<T,N>::getParams (void) const
{
    return this->params;
}

template <typename T, int N>
inline JacobiSolverParams<T>
JacobiSolver<T,N>::getInfo (void) const
{
    return this->resultInfo;
}

template <typename T, int N>
inline Vector<T,N>
JacobiSolver<T,N>::solve (Vector<T,N> const& rhs, Vector<T,N> const& initialX)
{
    Vector<T,N> currentX = initialX;
    Vector<T,N> lastX;

    int iteration = 0;
    T norm;
    for (; iteration < this->params.maxIter; iteration++) {
        lastX = currentX;
        currentX = rhs - this->nonDiagMatrix * lastX;

        norm = T(0);
        for (std::size_t i=0; i < N; i++) {
            /* compute i-th 'component' of ||b - Ax||^2 */
            T tmp = currentX[i] - this->diagonal[i] * lastX[i];
            norm += tmp * tmp;

            /* now we can update */
            currentX[i] /= this->diagonal[i];
        }

        if (norm < this->params.minResidual) {
            break;
        }
    }

    // save info
    JacobiSolverParams<T> info;
    info.maxIter = iteration;
    info.minResidual = norm;
    this->resultInfo = info;

    return currentX;
}

MATH_NAMESPACE_END

#endif /* MATH_JACOBISOLVER_HEADER */
