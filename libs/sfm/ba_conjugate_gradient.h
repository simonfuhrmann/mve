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

template <typename T>
class ConjugateGradient
{
public:
    typedef SparseMatrix<T> Matrix;
    typedef DenseVector<T> Vector;

    enum ReturnInfo
    {
        CG_CONVERGENCE,
        CG_MAX_ITERATIONS,
        CG_INVALID_INPUT
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
        virtual Vector multiply (Vector const& x) const = 0;
        virtual std::size_t input_size (void) const = 0;
        virtual std::size_t output_size (void) const = 0;
    };

public:
    ConjugateGradient (Options const& opts);

    Status solve (Matrix const& A, Vector const& b, Vector* x,
        Matrix const* P = nullptr);

    Status solve (Functor const& A, Vector const& b, Vector* x,
        Functor const* P = nullptr);

private:
    Options opts;
    Status status;
};

template <typename T>
class CGBasicMatrixFunctor : public ConjugateGradient<T>::Functor
{
public:
    CGBasicMatrixFunctor (SparseMatrix<T> const& A);
    DenseVector<T> multiply (DenseVector<T> const& x) const;
    std::size_t input_size (void) const;
    std::size_t output_size (void) const;

private:
    SparseMatrix<T> const* A;
};

/* ------------------------ Implementation ------------------------ */

template <typename T>
inline
ConjugateGradient<T>::Options::Options (void)
    : max_iterations(100)
    , tolerance(1e-20)
{
}

template <typename T>
inline
ConjugateGradient<T>::Status::Status (void)
    : num_iterations(0)
{
}

template <typename T>
inline
ConjugateGradient<T>::ConjugateGradient
    (Options const& options)
    : opts(options)
{
}

template <typename T>
inline typename ConjugateGradient<T>::Status
ConjugateGradient<T>::solve(Matrix const& A, Vector const& b, Vector* x,
    Matrix const* P)
{
    CGBasicMatrixFunctor<T> A_functor(A);
    CGBasicMatrixFunctor<T> P_functor(*P);
    return this->solve(A_functor, b, x, P == nullptr ? nullptr : &P_functor);
}

template <typename T>
typename ConjugateGradient<T>::Status
ConjugateGradient<T>::solve(Functor const& A, Vector const& b, Vector* x,
    Functor const* P)
{
    if (x == nullptr)
        throw std::invalid_argument("RHS must not be null");

    if (A.output_size() != b.size())
    {
        this->status.info = CG_INVALID_INPUT;
        return this->status;
    }

    /* Set intial x = 0. */
    if (x->size() != A.input_size())
    {
        x->clear();
        x->resize(A.input_size(), T(0));
    }
    else
    {
        x->fill(T(0));
    }

    /* Initial residual is r = b - Ax with x = 0. */
    Vector r = b;

    /* Regular search direction. */
    Vector d;
    /* Preconditioned search direction. */
    Vector z;
    /* Norm of residual. */
    T r_dot_r;

    /* Compute initial search direction. */
    if (P == nullptr)
    {
        r_dot_r = r.dot(r);
        d = r;
    }
    else
    {
        z = (*P).multiply(r);
        r_dot_r = z.dot(r);
        d = z;
    }

    for (this->status.num_iterations = 0;
        this->status.num_iterations < this->opts.max_iterations;
        this->status.num_iterations += 1)
    {
        /* Compute step size in search direction. */
        Vector Ad = A.multiply(d);
        T alpha = r_dot_r / d.dot(Ad);

        /* Update parameter vector. */
        *x = (*x).add(d.multiply(alpha));

        /* Compute new residual and its norm. */
        r = r.subtract(Ad.multiply(alpha));
        T new_r_dot_r = r.dot(r);

        /* Check tolerance condition. */
        if (new_r_dot_r < this->opts.tolerance)
        {
            this->status.info = CG_CONVERGENCE;
            return this->status;
        }

        /* Precondition residual if necessary. */
        if (P != nullptr)
        {
            z = P->multiply(r);
            new_r_dot_r = z.dot(r);
        }

        /*
         * Update search direction.
         * The next residual will be orthogonal to new Krylov space.
         */
        T beta = new_r_dot_r / r_dot_r;
        if (P != nullptr)
            d = z.add(d.multiply(beta));
        else
            d = r.add(d.multiply(beta));

        /* Update residual norm. */
        r_dot_r = new_r_dot_r;
    }

    this->status.info = CG_MAX_ITERATIONS;
    return this->status;
}

/* ---------------------------------------------------------------- */

template <typename T>
inline
CGBasicMatrixFunctor<T>::CGBasicMatrixFunctor (SparseMatrix<T> const& A)
    : A(&A)
{
}

template <typename T>
inline DenseVector<T>
CGBasicMatrixFunctor<T>::multiply (DenseVector<T> const& x) const
{
    return this->A->multiply(x);
}

template <typename T>
inline std::size_t
CGBasicMatrixFunctor<T>::input_size (void) const
{
    return A->num_cols();
}

template <typename T>
inline std::size_t
CGBasicMatrixFunctor<T>::output_size (void) const
{
    return A->num_rows();
}

SFM_NAMESPACE_END
SFM_BA_NAMESPACE_END

#endif /* SFM_BA_CONJUGATE_GRADIENT_HEADER */
