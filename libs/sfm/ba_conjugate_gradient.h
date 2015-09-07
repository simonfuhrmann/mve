/*
 * Copyright (C) 2015, Fabian Langguth, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef SFM_BA_CONJUGATE_GRADIENT_HEADER
#define SFM_BA_CONJUGATE_GRADIENT_HEADER

#include "sfm/defines.h"
#include "sfm/ba_dense_vector.h"
#include "sfm/ba_sparse_matrix.h"

SFM_NAMESPACE_BEGIN
SFM_BA_NAMESPACE_BEGIN

template<typename T>
class ConjugateGradientSolver
{
public:
    typedef SparseMatrix<T> Matrix;
    typedef DenseVector<T> Vector;

    enum ReturnInfo
    {
        CONVERGENCE,
        MAX_ITERATIONS,
        INVALID_INPUT
    };

    struct Options
    {
        Options (void);
        int max_iterations;
        T tolerance;
    };

    struct Status
    {
        Status (void);
        int num_iterations;
        ReturnInfo info;
    };

    class Functor
    {
    public:
        virtual Vector operator() (Vector const& x) const = 0;
        virtual std::size_t input_size (void) const = 0;
        virtual std::size_t output_size (void) const = 0;
    };

    class BasicMatrixFunctor : public Functor
    {
    public:
        BasicMatrixFunctor(Matrix const* A)
        : A(A) {}

        Vector operator()(Vector const& x) const
        {
            return this->A->multiply(x);
        }
        std::size_t input_size (void) const
        {
            return A->num_cols();
        }
        std::size_t output_size (void) const
        {
            return A->num_rows();
        }

    private:
        Matrix const* A;
    };


public:
    ConjugateGradientSolver(Options const& opts);

    Status solve (Matrix const& A, Vector const& b, Vector* x,
        Matrix const* P = nullptr);

    Status solve (Functor const& A, Vector const& b, Vector* x,
        Matrix const* P = nullptr);

private:
    Options opts;
    Status status;
};


/* ------------------------ Implementation ------------------------ */

template<typename T>
inline
ConjugateGradientSolver<T>::Options::Options (void)
    : max_iterations(100)
    , tolerance(1e-40)
{
}

template<typename T>
inline
ConjugateGradientSolver<T>::Status::Status (void)
    : num_iterations(0)
{
}

template<typename T>
inline
ConjugateGradientSolver<T>::ConjugateGradientSolver
    (Options const& options)
    : opts(options)
{
}

template<typename T>
inline typename ConjugateGradientSolver<T>::Status
ConjugateGradientSolver<T>::solve(Matrix const& A, Vector const& b, Vector* x,
    Matrix const* P)
{
    BasicMatrixFunctor A_functor(&A);
    return this->solve(A_functor, b, x, P);
}

template<typename T>
inline typename ConjugateGradientSolver<T>::Status
ConjugateGradientSolver<T>::solve(ConjugateGradientSolver<T>::Functor const& A,
    Vector const& b, Vector* x, Matrix const* P)
{
    if (A.output_size() != b.size())
    {
        this->status.info = INVALID_INPUT;
        return this->status;
    }

    /* Set intial x = 0 */
    x->resize(A.input_size(), 0);
    /* Initial residual is b */
    Vector r = b;
    /* Regular search direction */
    Vector d;
    /* Preconditioned search direction */
    Vector z;
    /* Norm of residual */
    T r_dot_r;

    /* Compute initial search direction */
    if(P)
    {
        z = (*P).multiply(r);
        r_dot_r = z.dot(r);
        d = z;
    }
    else
    {
        r_dot_r = r.dot(r);
        d = b;
    }

    this->status.num_iterations = 0;
    for ( ; this->status.num_iterations < this->opts.max_iterations;
        ++this->status.num_iterations)
    {
        /* Compute step size in search direction */
        Vector Ad = A(d);
        T alpha = r_dot_r / d.dot(Ad);

        /* Update parameter vector */
        *x = (*x).add(d.multiply(alpha));

        /* Compute new residual and its norm */
        r = r.subtract(Ad.multiply(alpha));
        T new_r_dot_r = r.dot(r);

        /* Check tolerance condition */
        if(new_r_dot_r < this->opts.tolerance)
        {
            this->status.info = CONVERGENCE;
            return this->status;
        }

        /* Precondition residual if necessary */
        if (P)
        {
            z = (*P).multiply(r);
            new_r_dot_r = z.dot(r);
        }

        /* Update search direction s.t. the next residual will be
           orthogonal to new Krylov space */
        T beta = new_r_dot_r / r_dot_r;
        if (P)
            d = z.add(d.multiply(beta));
        else
            d = r.add(d.multiply(beta));

        /* Update residual norm */
        r_dot_r = new_r_dot_r;
    }

    this->status.info = MAX_ITERATIONS;
    return this->status;
}

SFM_NAMESPACE_END
SFM_BA_NAMESPACE_END

#endif /* SFM_BA_CONJUGATE_GRADIENT_HEADER */
