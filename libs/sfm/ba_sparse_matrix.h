/*
 * Copyright (C) 2015, Simon Fuhrmann, Fabian Langguth
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef SFM_SPARSE_MATRIX_HEADER
#define SFM_SPARSE_MATRIX_HEADER

#include <thread>
#include <stdexcept>
#include <vector>
#include <algorithm>

#include "sfm/ba_dense_vector.h"
#include "sfm/defines.h"

SFM_NAMESPACE_BEGIN
SFM_BA_NAMESPACE_BEGIN

/**
 * Sparse matrix class in Yale format for column-major matrices.
 */
template <typename T>
class SparseMatrix
{
public:
    /** Triplet with row/col index, and the actual value. */
    struct Triplet
    {
        Triplet (void) = default;
        Triplet (std::size_t row, std::size_t col, T const& value);

        std::size_t row;
        std::size_t col;
        T value;
    };

    /** List of triplets. */
    typedef std::vector<Triplet> Triplets;

public:
    SparseMatrix (void);
    SparseMatrix (std::size_t rows, std::size_t cols);
    void allocate (std::size_t rows, std::size_t cols);
    void reserve (std::size_t num_elements);
    void set_from_triplets (Triplets const& triplets);
    void mult_diagonal (T const& factor);
    void cwise_invert (void);
    void column_nonzeros (std::size_t col, DenseVector<T>* vector) const;

    SparseMatrix transpose (void) const;
    SparseMatrix subtract (SparseMatrix const& rhs) const;
    SparseMatrix multiply (SparseMatrix const& rhs) const;
    SparseMatrix sequential_multiply (SparseMatrix const& rhs) const;
    SparseMatrix parallel_multiply (SparseMatrix const& rhs) const;
    DenseVector<T> multiply (DenseVector<T> const& rhs) const;
    SparseMatrix diagonal_matrix (void) const;

    std::size_t num_non_zero (void) const;
    std::size_t num_rows (void) const;
    std::size_t num_cols (void) const;
    T* begin (void);
    T* end (void);

    void debug (void) const;

private:
    std::size_t rows;
    std::size_t cols;
    std::vector<T> values;
    std::vector<std::size_t> outer;
    std::vector<std::size_t> inner;
};

SFM_BA_NAMESPACE_END
SFM_NAMESPACE_END

/* ------------------------ Implementation ------------------------ */

#include <iostream>

SFM_NAMESPACE_BEGIN
SFM_BA_NAMESPACE_BEGIN

template <typename T>
SparseMatrix<T>::Triplet::Triplet (std::size_t row,
    std::size_t col, T const& value)
    : row(row), col(col), value(value)
{
}

/* --------------------------------------------------------------- */

template <typename T>
SparseMatrix<T>::SparseMatrix (void)
    : rows(0)
    , cols(0)
{
}

template <typename T>
SparseMatrix<T>::SparseMatrix (std::size_t rows, std::size_t cols)
{
    this->allocate(rows, cols);
}

template <typename T>
void
SparseMatrix<T>::allocate (std::size_t rows, std::size_t cols)
{
    this->rows = rows;
    this->cols = cols;
    this->values.clear();
    this->outer.clear();
    this->inner.clear();
    this->outer.resize(cols + 1, 0);
}

template <typename T>
void
SparseMatrix<T>::reserve (std::size_t num_elements)
{
    this->inner.reserve(num_elements);
    this->values.reserve(num_elements);
}

template <typename T>
void
SparseMatrix<T>::set_from_triplets (Triplets const& triplets)
{
    /* Create a temporary transposed matrix */
    SparseMatrix<T> transposed(this->cols, this->rows);
    transposed.values.resize(triplets.size());
    transposed.inner.resize(triplets.size());

    /* Initialize outer indices with amount of inner values. */
    for (std::size_t i = 0; i < triplets.size(); ++i)
        transposed.outer[triplets[i].row]++;

    /* Convert amounts to indices with prefix sum. */
    std::size_t sum = 0;
    std::vector<std::size_t> scratch(transposed.outer.size());
    for (std::size_t i = 0; i < transposed.outer.size(); ++i)
    {
        std::size_t const temp = transposed.outer[i];
        transposed.outer[i] = sum;
        scratch[i] = sum;
        sum += temp;
    }

    /* Add triplets, inner indices are unsorted. */
    for (std::size_t i = 0; i < triplets.size(); ++i)
    {
        Triplet const& t = triplets[i];
        std::size_t pos = scratch[t.row]++;
        transposed.values[pos] = t.value;
        transposed.inner[pos] = t.col;
    }

    /* Transpose matrix, implicit sorting of inner indices. */
    *this = transposed.transpose();
}

template <typename T>
SparseMatrix<T>
SparseMatrix<T>::transpose (void) const
{
    SparseMatrix ret(this->cols, this->rows);
    ret.values.resize(this->num_non_zero());
    ret.inner.resize(this->num_non_zero());

    /* Compute inner sizes of transposed matrix. */
    for(std::size_t i = 0; i < this->inner.size(); ++i)
        ret.outer[this->inner[i]] += 1;

    /* Compute outer sizes of transposed matrix with prefix sum. */
    std::size_t sum = 0;
    std::vector<std::size_t> scratch(ret.outer.size());
    for (std::size_t i = 0; i < ret.outer.size(); ++i)
    {
        std::size_t const temp = ret.outer[i];
        ret.outer[i] = sum;
        scratch[i] = sum;
        sum += temp;
    }

    /* Write inner indices and values of transposed matrix. */
    for (std::size_t i = 0; i < this->outer.size() - 1; ++i)
        for (std::size_t j = this->outer[i]; j < this->outer[i + 1]; ++j)
        {
            std::size_t pos = scratch[this->inner[j]]++;
            ret.inner[pos] = i;
            ret.values[pos] = this->values[j];
        }

    return ret;
}

template <typename T>
SparseMatrix<T>
SparseMatrix<T>::subtract (SparseMatrix const& rhs) const
{
    if (this->rows != rhs.rows || this->cols != rhs.cols)
        throw std::invalid_argument("Incompatible matrix dimensions");

    SparseMatrix ret(this->rows, this->cols);
    ret.reserve(this->num_non_zero() + rhs.num_non_zero());

    std::size_t num_outer = this->outer.size() - 1;
    for (std::size_t outer = 0; outer < num_outer; ++outer)
    {
        ret.outer[outer] = ret.values.size();

        std::size_t i1 = this->outer[outer];
        std::size_t i2 = rhs.outer[outer];
        std::size_t const i1_end = this->outer[outer + 1];
        std::size_t const i2_end = rhs.outer[outer + 1];
        while (i1 < i1_end || i2 < i2_end)
        {
            if (i1 >= i1_end)
            {
                ret.values.push_back(-rhs.values[i2]);
                ret.inner.push_back(rhs.inner[i2]);
                i2 += 1;
                continue;
            }
            if (i2 >= i2_end)
            {
                ret.values.push_back(this->values[i1]);
                ret.inner.push_back(this->inner[i1]);
                i1 += 1;
                continue;
            }

            std::size_t id1 = this->inner[i1];
            std::size_t id2 = rhs.inner[i2];

            if (id1 < id2)
                ret.values.push_back(this->values[i1]);
            else if (id2 < id1)
                ret.values.push_back(-rhs.values[i2]);
            else
                ret.values.push_back(this->values[i1] - rhs.values[i2]);

            i1 += static_cast<std::size_t>(id1 <= id2);
            i2 += static_cast<std::size_t>(id2 <= id1);
            ret.inner.push_back(std::min(id1, id2));
        }
    }
    ret.outer.back() = ret.values.size();

    return ret;
}

template <typename T>
SparseMatrix<T>
SparseMatrix<T>::multiply (SparseMatrix const& rhs) const
{
#ifdef _OPENMP
    return this->parallel_multiply(rhs);
#else
    return this->sequential_multiply(rhs);
#endif
}

template <typename T>
SparseMatrix<T>
SparseMatrix<T>::sequential_multiply (SparseMatrix const& rhs) const
{
    if (this->cols != rhs.rows)
        throw std::invalid_argument("Incompatible matrix dimensions");

    SparseMatrix ret(this->rows, rhs.cols);
    ret.reserve(this->num_non_zero() + rhs.num_non_zero());

    /* Matrix-matrix multiplication. */
    std::vector<T> ret_col(ret.rows, T(0));
    std::vector<bool> ret_nonzero(ret.rows, false);
    for (std::size_t col = 0; col < ret.cols; ++col)
    {
        ret.outer[col] = ret.values.size();

        std::fill(ret_col.begin(), ret_col.end(), T(0));
        std::fill(ret_nonzero.begin(), ret_nonzero.end(), false);
        std::size_t rhs_col_begin = rhs.outer[col];
        std::size_t rhs_col_end = rhs.outer[col + 1];
        for (std::size_t i = rhs_col_begin; i < rhs_col_end; ++i)
        {
            T const& rhs_col_value = rhs.values[i];
            std::size_t const lhs_col = rhs.inner[i];
            std::size_t const lhs_col_begin = this->outer[lhs_col];
            std::size_t const lhs_col_end = this->outer[lhs_col + 1];

            for (std::size_t j = lhs_col_begin; j < lhs_col_end; ++j)
            {
                std::size_t const id = this->inner[j];
                ret_col[id] += this->values[j] * rhs_col_value;
                ret_nonzero[id] = true;
            }
        }
        for (std::size_t i = 0; i < ret.rows; ++i)
            if (ret_nonzero[i])
            {
                ret.inner.push_back(i);
                ret.values.push_back(ret_col[i]);
            }
    }
    ret.outer[ret.cols] = ret.values.size();

    return ret;
}

template <typename T>
SparseMatrix<T>
SparseMatrix<T>::parallel_multiply (SparseMatrix const& rhs) const
{
    if (this->cols != rhs.rows)
        throw std::invalid_argument("Incompatible matrix dimensions");

    std::size_t nnz = this->num_non_zero() + rhs.num_non_zero();
    SparseMatrix ret(this->rows, rhs.cols);
    ret.reserve(nnz);
    std::fill(ret.outer.begin(), ret.outer.end(), 0);

    std::size_t const chunk_size = 64;
    std::size_t const num_chunks = ret.cols / chunk_size
         + (ret.cols % chunk_size != 0);
    std::size_t const max_threads = std::max(1u,
        std::thread::hardware_concurrency());
    std::size_t const num_threads = std::min(num_chunks, max_threads);

#pragma omp parallel num_threads(num_threads)
    {
        /* Matrix-matrix multiplication. */
        std::vector<T> ret_col(ret.rows, T(0));
        std::vector<bool> ret_nonzero(ret.rows, false);
        std::vector<T> thread_values;
        thread_values.reserve(nnz / num_chunks);
        std::vector<std::size_t> thread_inner;
        thread_inner.reserve(nnz / num_chunks);

#pragma omp for ordered schedule(static, 1)
        for (std::size_t chunk = 0; chunk < num_chunks; ++chunk)
        {
            thread_inner.clear();
            thread_values.clear();

            std::size_t const begin = chunk * chunk_size;
            std::size_t const end = std::min(begin + chunk_size, ret.cols);
            for (std::size_t col = begin; col < end; ++col)
            {
                std::fill(ret_col.begin(), ret_col.end(), T(0));
                std::fill(ret_nonzero.begin(), ret_nonzero.end(), false);
                std::size_t const rhs_col_begin = rhs.outer[col];
                std::size_t const rhs_col_end = rhs.outer[col + 1];
                for (std::size_t i = rhs_col_begin; i < rhs_col_end; ++i)
                {
                    T const& rhs_col_value = rhs.values[i];
                    std::size_t const lhs_col = rhs.inner[i];
                    std::size_t const lhs_col_begin = this->outer[lhs_col];
                    std::size_t const lhs_col_end = this->outer[lhs_col + 1];

                    for (std::size_t j = lhs_col_begin; j < lhs_col_end; ++j)
                    {
                        std::size_t const id = this->inner[j];
                        ret_col[id] += this->values[j] * rhs_col_value;
                        ret_nonzero[id] = true;
                    }
                }
                for (std::size_t i = 0; i < ret.rows; ++i)
                    if (ret_nonzero[i])
                    {
                        ret.outer[col + 1] += 1;
                        thread_inner.push_back(i);
                        thread_values.push_back(ret_col[i]);
                    }
            }

#pragma omp ordered
            {
                ret.inner.insert(ret.inner.end(),
                    thread_inner.begin(), thread_inner.end());
                ret.values.insert(ret.values.end(),
                    thread_values.begin(), thread_values.end());
            }
        }
    }

    for (std::size_t col = 0; col < ret.cols; ++col)
        ret.outer[col + 1] += ret.outer[col];

    return ret;
}

template<typename T>
DenseVector<T>
SparseMatrix<T>::multiply (DenseVector<T> const& rhs) const
{
    if (rhs.size() != this->cols)
        throw std::invalid_argument("Incompatible dimensions");

    DenseVector<T> ret(this->rows, T(0));
    for (std::size_t i = 0; i < this->cols; ++i)
        for (std::size_t id = this->outer[i]; id < this->outer[i + 1]; ++id)
            ret[this->inner[id]] += this->values[id] * rhs[i];
    return ret;
}

template<typename T>
SparseMatrix<T>
SparseMatrix<T>::diagonal_matrix (void) const
{
    std::size_t const diag_size = std::min(this->rows, this->cols);
    SparseMatrix ret(diag_size, diag_size);
    ret.reserve(diag_size);
    for (std::size_t i = 0; i < diag_size; ++i)
    {
        ret.outer[i] = ret.values.size();
        for (std::size_t j = this->outer[i]; j < this->outer[i + 1]; ++j)
            if (this->inner[j] == i)
            {
                ret.inner.push_back(i);
                ret.values.push_back(this->values[j]);
            }
            else if (this->inner[j] > i)
                break;
    }
    ret.outer[diag_size] = ret.values.size();
    return ret;
}

template<typename T>
void
SparseMatrix<T>::mult_diagonal (T const& factor)
{
    for (std::size_t i = 0; i < this->outer.size() - 1; ++i)
        for (std::size_t j = this->outer[i]; j < this->outer[i + 1]; ++j)
        {
            if (this->inner[j] == i)
                this->values[j] *= factor;
            if (this->inner[j] >= i)
                break;
        }
}

template<typename T>
void
SparseMatrix<T>::cwise_invert (void)
{
    for (std::size_t i = 0; i < this->values.size(); ++i)
        this->values[i] = T(1) / this->values[i];
}

template<typename T>
void
SparseMatrix<T>::column_nonzeros (std::size_t col, DenseVector<T>* vector) const
{
    std::size_t const start = this->outer[col];
    std::size_t const end = this->outer[col + 1];
    vector->resize(end - start);
    for (std::size_t row = start, i = 0; row < end; ++row, ++i)
        vector->at(i) = this->values[row];
}

template<typename T>
inline std::size_t
SparseMatrix<T>::num_non_zero (void) const
{
    return this->values.size();
}

template<typename T>
inline std::size_t
SparseMatrix<T>::num_rows (void) const
{
    return this->rows;
}

template<typename T>
inline std::size_t
SparseMatrix<T>::num_cols (void) const
{
    return this->cols;
}

template<typename T>
inline T*
SparseMatrix<T>::begin (void)
{
    return this->values.data();
}

template<typename T>
inline T*
SparseMatrix<T>::end (void)
{
    return this->values.data() + this->values.size();
}

template<typename T>
void
SparseMatrix<T>::debug (void) const
{
    std::cout << "SparseMatrix ("
        << this->rows << " rows, " << this->cols << " cols, "
        << this->num_non_zero() << " values)" << std::endl;
    std::cout << "  Values:";
    for (std::size_t i = 0; i < this->values.size(); ++i)
        std::cout << " " << this->values[i];
    std::cout << std::endl << "  Inner:";
    for (std::size_t i = 0; i < this->inner.size(); ++i)
        std::cout << " " << this->inner[i];
    std::cout << std::endl << "  Outer:";
    for (std::size_t i = 0; i < this->outer.size(); ++i)
        std::cout << " " << this->outer[i];
    std::cout << std::endl;
}

SFM_BA_NAMESPACE_END
SFM_NAMESPACE_END

#endif // SFM_SPARSE_MATRIX_HEADER

