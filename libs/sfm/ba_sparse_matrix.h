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

#include <stdexcept>
#include <vector>
#include <memory>
#include <algorithm>

#include "sfm/ba_dense_vector.h"
#include "sfm/defines.h"

SFM_NAMESPACE_BEGIN
SFM_BA_NAMESPACE_BEGIN

/**
 * Sparse matrix class with compressed row/column storage for both,
 * row-major and column-major matrices. It supports
 *
 * - Cheap copy operation (by sharing of data)
 * - Cheap transpose operation (by sharing data and changing majorness)
 * - expensive layout change (from row- to column-major and vice versa)
 * - setting values from triplets (by sorting the triplets)
 * - multiplication of compatible matrices (row- with column-major)
 * - multiplication of diagonal entries with a factor
 * - subtraction of compatible matrices (row/row- and col/col-major)
 *
 * Implicit expensive conversions for operations on incompatible types
 * (like multiplication, subtraction) are not supported.
 */
template <typename T>
class SparseMatrix
{
public:
    /*
     * Data type indicating whether consecutive matrix elements in memory
     * form matrix rows (row major) or columns (column major).
     */
    enum DataLayout
    {
        ROW_MAJOR,
        COLUMN_MAJOR
    };

    /**
     * Triplet with inner and outer index, and the actual value.
     * Indexing depends on majorness, i.e., the same triplets on two
     * matrices with different majorness will lead to different matrices.
     *
     * - Row-major: outer = rows, inner = cols
     * - Col-major: outer = cols, inner = rows
     *
     * The comparison orders by outer index, then by inner index.
     */
    struct Triplet
    {
        Triplet (void) = default;
        Triplet (std::size_t outer, std::size_t inner, T const& value);
        bool operator< (Triplet const& other) const;

        std::size_t outer;
        std::size_t inner;
        T value;
    };

    typedef std::vector<Triplet> Triplets;

public:
    SparseMatrix (void);
    SparseMatrix (std::size_t rows, std::size_t cols, DataLayout layout);
    void allocate (std::size_t rows, std::size_t cols, DataLayout layout);
    void reserve (std::size_t num_elements);
    void set_from_triplets (Triplets* triplets);
    bool is_shared (void) const;
    void copy_if_shared (void);

    SparseMatrix change_layout (void) const;
    SparseMatrix transpose (void) const;
    SparseMatrix subtract (SparseMatrix const& rhs) const;
    SparseMatrix multiply (SparseMatrix const& rhs) const;
    DenseVector<T> multiply (DenseVector<T> const& rhs) const;
    void mult_diagonal (T const& factor);

    std::size_t num_non_zero (void) const;
    std::size_t num_rows (void) const;
    std::size_t num_cols (void) const;
    DataLayout get_layout (void) const;
    T* begin (void);
    T* end (void);

    void debug (void) const;

private:
    struct MatrixData
    {
        typedef std::shared_ptr<MatrixData> Ptr;

        void init_outer (std::size_t outer_size);
        void reserve (std::size_t num_elements);
        Triplets to_triplets (void) const;
        void from_triplets (Triplets const& triplets);

        std::vector<T> values;
        std::vector<std::size_t> outer;
        std::vector<std::size_t> inner;
    };

private:
    SparseMatrix multiply_intern (SparseMatrix const& sm);

private:
    std::size_t rows;
    std::size_t cols;
    DataLayout layout;
    typename MatrixData::Ptr data;
};


/* ------------------------ Implementation ------------------------ */

template <typename T>
SparseMatrix<T>::Triplet::Triplet (std::size_t outer,
    std::size_t inner, T const& value)
    : outer(outer) , inner(inner), value(value)
{
}

template <typename T>
bool
SparseMatrix<T>::Triplet::operator< (Triplet const& other) const
{
    return this->outer < other.outer
        || (this->outer == other.outer && this->inner < other.inner);
}

template <typename T>
void
SparseMatrix<T>::MatrixData::init_outer (std::size_t outer_size)
{
    this->values.clear();
    this->inner.clear();
    this->outer.resize(outer_size + 1, 0);
}

template <typename T>
void
SparseMatrix<T>::MatrixData::reserve (std::size_t num_elements)
{
    this->values.reserve(num_elements);
    this->inner.reserve(num_elements);
}

template <typename T>
typename SparseMatrix<T>::Triplets
SparseMatrix<T>::MatrixData::to_triplets (void) const
{
    Triplets ret;
    ret.reserve(this->values.size());
    for (std::size_t i = 0; i < this->outer.size() - 1; ++i)
        for (std::size_t j = this->outer[i]; j < this->outer[i + 1]; ++j)
            ret.emplace_back(this->inner[j], i, this->values[j]);
    return ret;
}

template <typename T>
void
SparseMatrix<T>::MatrixData::from_triplets (Triplets const& triplets)
{
    this->values.clear();
    this->inner.clear();
    this->reserve(triplets.size());

    std::size_t outer = 0;
    for (std::size_t i = 0; i < triplets.size(); ++i)
    {
        Triplet const& t = triplets[i];
        for ( ; outer <= t.outer; ++outer)
            this->outer[outer] = i;
        this->values.push_back(t.value);
        this->inner.push_back(t.inner);
    }
    for (std::size_t j = outer; j < this->outer.size(); ++j)
        this->outer[j] = triplets.size();
}

/* ------------------------ Implementation ------------------------ */

template <typename T>
SparseMatrix<T>::SparseMatrix (void)
    : rows(0)
    , cols(0)
    , layout(ROW_MAJOR)
{
}

template <typename T>
SparseMatrix<T>::SparseMatrix (std::size_t rows, std::size_t cols,
    DataLayout layout)
{
    this->allocate(rows, cols, layout);
}

template <typename T>
void
SparseMatrix<T>::allocate (std::size_t rows, std::size_t cols,
    DataLayout layout)
{
    this->rows = rows;
    this->cols = cols;
    this->layout = layout;
    this->data.reset(new MatrixData);
    this->data->init_outer(layout == ROW_MAJOR ? rows : cols);
}

template <typename T>
void
SparseMatrix<T>::reserve (std::size_t num_elements)
{
    if (this->data != nullptr)
        this->data->reserve(num_elements);
}

template <typename T>
void
SparseMatrix<T>::set_from_triplets (Triplets* triplets)
{
    std::sort(triplets->begin(), triplets->end());
    this->data->from_triplets(*triplets);
}

template <typename T>
SparseMatrix<T>
SparseMatrix<T>::change_layout (void) const
{
    Triplets triplets = this->data->to_triplets();

    SparseMatrix ret(this->rows, this->cols,
        this->layout == ROW_MAJOR ? COLUMN_MAJOR : ROW_MAJOR);
    ret.set_from_triplets(&triplets);
    return ret;
}

template<typename T>
bool
SparseMatrix<T>::is_shared (void) const
{
    return this->data.use_count() > 1;
}

template<typename T>
void
SparseMatrix<T>::copy_if_shared (void)
{
    if (this->is_shared())
        this->data.reset(new MatrixData(*this->data));
}

template <typename T>
SparseMatrix<T>
SparseMatrix<T>::transpose (void) const
{
    SparseMatrix ret;
    ret.layout = this->layout == ROW_MAJOR ? COLUMN_MAJOR : ROW_MAJOR;
    ret.rows = this->cols;
    ret.cols = this->rows;
    ret.data = data;
    return ret;
}

template <typename T>
SparseMatrix<T>
SparseMatrix<T>::subtract (SparseMatrix const& rhs) const
{
    if (this->layout != rhs.layout)
        throw std::invalid_argument("Operands with different majorness");
    if (this->rows != rhs.rows || this->cols != rhs.cols)
        throw std::invalid_argument("Incompatible matrix dimensions");

    SparseMatrix ret(this->rows, this->cols, this->layout);
    ret.reserve(std::max(this->num_non_zero(), rhs.num_non_zero()));

    std::size_t num_outer = this->data->outer.size() - 1;
    for (std::size_t outer = 0; outer < num_outer; ++outer)
    {
        ret.data->outer[outer] = ret.data->values.size();

        std::size_t i1 = this->data->outer[outer];
        std::size_t i2 = rhs.data->outer[outer];
        std::size_t const i1_end = this->data->outer[outer + 1];
        std::size_t const i2_end = rhs.data->outer[outer + 1];
        while (i1 < i1_end || i2 < i2_end)
        {
            if (i1 >= i1_end)
            {
                ret.data->values.push_back(-rhs.data->values[i2]);
                ret.data->inner.push_back(rhs.data->inner[i2]);
                i2 += 1;
                continue;
            }
            if (i2 >= i2_end)
            {
                ret.data->values.push_back(this->data->values[i1]);
                ret.data->inner.push_back(this->data->inner[i1]);
                i1 += 1;
                continue;
            }

            std::size_t id1 = this->data->inner[i1];
            std::size_t id2 = rhs.data->inner[i2];

            if (id1 < id2)
                ret.data->values.push_back(this->data->values[i1]);
            else if (id2 < id1)
                ret.data->values.push_back(-rhs.data->values[i2]);
            else
                ret.data->values.push_back(this->data->values[i1]
                    - rhs.data->values[i2]);

            i1 += static_cast<std::size_t>(id1 <= id2);
            i2 += static_cast<std::size_t>(id2 <= id1);
            ret.data->inner.push_back(std::min(id1, id2));
        }
    }
    ret.data->outer.back() = ret.data->values.size();

    return ret;
}

template <typename T>
SparseMatrix<T>
SparseMatrix<T>::multiply (SparseMatrix const& rhs) const
{
    if (this->layout != ROW_MAJOR)
        throw std::invalid_argument("LHS must be in row-major");
    if (rhs.layout != COLUMN_MAJOR)
        throw std::invalid_argument("RHS must be in column-major");
    if (this->cols != rhs.rows)
        throw std::invalid_argument("Incompatible matrix dimensions");

    SparseMatrix ret(this->rows, rhs.cols, ROW_MAJOR);
    ret.data->reserve(std::min(this->num_non_zero(), rhs.num_non_zero()));

    /* Matrix-matrix multiplication. */
    for (std::size_t row = 0; row < ret.rows; ++row)
    {
        ret.data->outer[row] = ret.data->values.size();
        for (std::size_t col = 0; col < ret.cols; ++col)
        {
            T value(0);
            bool is_nonzero = false;

            /* Dot product for sparse row/column. */
            std::size_t i1 = this->data->outer[row];
            std::size_t i2 = rhs.data->outer[col];
            for (; i1 < this->data->outer[row + 1]
                && i2 < rhs.data->outer[col + 1]; )
            {
                std::size_t id1 = this->data->inner[i1];
                std::size_t id2 = rhs.data->inner[i2];
                if (id1 == id2)
                    value += this->data->values[i1] * rhs.data->values[i2];
                is_nonzero |= (id1 == id2);
                i1 += static_cast<std::size_t>(id1 <= id2);
                i2 += static_cast<std::size_t>(id2 <= id1);
            }

            /* Update target matrix if dot product is non-zero. */
            if (is_nonzero)
            {
                ret.data->values.push_back(value);
                ret.data->inner.push_back(col);
            }
        }
    }
    ret.data->outer[ret.rows] = ret.data->values.size();

    return ret;
}

template<typename T>
DenseVector<T>
SparseMatrix<T>::multiply (DenseVector<T> const& rhs) const
{
    if (this->layout != ROW_MAJOR)
        throw std::invalid_argument("LHS must be in row-major");
    if (rhs.size() != this->cols)
        throw std::invalid_argument("Incompatible dimensions");

    DenseVector<T> ret(this->rows, T(0));
    for (std::size_t i = 0; i < this->rows; ++i)
    {
        std::size_t const i1 = this->data->outer[i];
        std::size_t const i2 = this->data->outer[i + 1];
        for (std::size_t id = i1; id < i2; ++id)
            ret[i] += this->data->values[id] * rhs[this->data->inner[id]];
    }
    return ret;
}


template<typename T>
void
SparseMatrix<T>::mult_diagonal (T const& factor)
{
    std::vector<std::size_t> const& outer = this->data->outer;
    std::vector<std::size_t> const& inner = this->data->inner;

    for (std::size_t i = 0; i < outer.size() - 1; ++i)
        for (std::size_t j = outer[i]; j < outer[i + 1]; ++j)
        {
            if (inner[j] == i)
                this->data->values[j] *= factor;
            if (inner[j] >= i)
                break;
        }
}

template<typename T>
inline std::size_t
SparseMatrix<T>::num_non_zero (void) const
{
    return this->data == nullptr ? 0 : this->data->values.size();
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
typename SparseMatrix<T>::DataLayout
SparseMatrix<T>::get_layout (void) const
{
    return this->layout;
}

template<typename T>
inline T*
SparseMatrix<T>::begin (void)
{
    return this->data->values.data();
}

template<typename T>
inline T*
SparseMatrix<T>::end (void)
{
    return this->data->values.data() + this->data->values.size();
}

#if 1
template<typename T>
void
SparseMatrix<T>::debug (void) const
{
    std::cout << "SparseMatrix (" << this->rows << " rows, "
        << this->cols << " cols, layout "
        << (this->layout == ROW_MAJOR ? "RM" : "CM") << ", "
        << (this->data == nullptr ? 0 : this->data->values.size())
        << " values)" << std::endl;

    if (this->data == nullptr)
        return;

    std::cout << "  Values:";
    for (std::size_t i = 0; i < this->data->values.size(); ++i)
        std::cout << " " << this->data->values[i];
    std::cout << std::endl << "  Inner:";
    for (std::size_t i = 0; i < this->data->inner.size(); ++i)
        std::cout << " " << this->data->inner[i];
    std::cout << std::endl << "  Outer:";
    for (std::size_t i = 0; i < this->data->outer.size(); ++i)
        std::cout << " " << this->data->outer[i];
    std::cout << std::endl;
}
#endif

SFM_BA_NAMESPACE_END
SFM_NAMESPACE_END

#endif // SFM_SPARSE_MATRIX_HEADER

