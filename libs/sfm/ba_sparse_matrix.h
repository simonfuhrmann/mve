#ifndef MATH_SPARSE_MATRIX_HEADER
#define MATH_SPARSE_MATRIX_HEADER

#include <stdexcept>
#include <vector>

#include "math/defines.h"

MATH_NAMESPACE_BEGIN

template <typename T>
class SparseMatrix
{
public:
    SparseMatrix (void);
    SparseMatrix (std::size_t num_rows, std::size_t num_cols);
    void resize (std::size_t num_rows, std::size_t num_cols);
    void reserve (std::size_t num_elements);
    std::size_t num_non_zero (void);

    T* push_back (std::size_t row, std::size_t col, std::size_t num = 1);
    T* begin (void);
    T* end (void);

    std::vector<T> operator* (std::vector<T> const& rhs) const;
    std::vector<T> mult (std::vector<T> const& rhs) const;

    SparseMatrix& mult_diagonal (T const& factor);
    SparseMatrix& transpose (void);

private:
    /** Number of matrix rows. */
    std::size_t rows;
    /** Number of matrix cols. */
    std::size_t cols;
    /** Row or column-major matrix. */
    bool transposed;
    /** Non-zero matrix values. */
    std::vector<T> values;
    /** Per-value column index. */
    std::vector<std::size_t> col_index;
    /** Per-row start ID into values array. */
    std::vector<std::size_t> row_ids;
};

/* ------------------------ Implementation ------------------------ */

template <typename T>
inline
SparseMatrix<T>::SparseMatrix (void)
    : rows(0)
    , cols(0)
    , transposed(false)
{
    this->row_ids.push_back(0);
}

template <typename T>
inline
SparseMatrix<T>::SparseMatrix (std::size_t num_rows, std::size_t num_cols)
{
    this->resize(num_rows, num_cols);
}

template <typename T>
inline void
SparseMatrix<T>::resize (std::size_t num_rows, std::size_t num_cols)
{
    this->rows = num_rows;
    this->cols = num_cols;
    this->values.clear();
    this->row_ids.clear();
    this->col_index.clear();
    this->row_ids.resize(this->rows + 1, 0);
}

template <typename T>
inline void
SparseMatrix<T>::reserve (std::size_t num_elements)
{
    this->values.reserve(num_elements);
}

template <typename T>
inline std::size_t
SparseMatrix<T>::num_non_zero (void)
{
    return this->values.size();
}

template <typename T>
inline T*
SparseMatrix<T>::push_back (std::size_t row, std::size_t col, std::size_t num)
{
    if (col + num >= this->cols)
        throw std::invalid_argument("Matrix column out of bounds");
    if (row >= this->rows)
        throw std::invalid_argument("Matrix rows out of bounds");
    if (num == 0)
        throw std::invalid_argument("Number must be at least one");
    if (this->row_ids[row] != this->values.size()
        && (this->row_ids[row + 1] != this->values.size()
        || this->col_index.back() >= col))
        throw std::invalid_argument("Inserted elements must be last");

    /* Reserve new storage. */
    std::size_t old_nnz = this->values.size();
    std::size_t new_reserved = std::max(this->values.capacity(), this->values.size() + num);
    this->values.reserve(new_reserved);
    this->col_index.reserve(new_reserved);

    /* Append values and per-value column ID. */
    for (std::size_t i = 0; i < num; ++i)
    {
        this->values.push_back(T());
        this->col_index.push_back(col + i);
    }

    /* Update row IDs. */
    for (std::size_t i = row + 1; i < this->row_ids.size(); ++i)
        this->row_ids[i] += num;

    return &this->values[0] + old_nnz;
}

template <typename T>
inline T*
SparseMatrix<T>::begin (void)
{
    return this->values.empty()
        ? NULL
        : this->values.data();
}

template <typename T>
inline T*
SparseMatrix<T>::end (void)
{
    return this->values.empty()
        ? NULL
        : this->values.data() + this->values.size();
}

template <typename T>
inline std::vector<T>
SparseMatrix<T>::operator* (std::vector<T> const& rhs) const
{
    this->mult(rhs);
}

template<typename T>
inline std::vector<T>
SparseMatrix<T>::mult (std::vector<T> const& rhs) const
{
    if (rhs.size() != this->cols)
        throw std::invalid_argument("Incompatible dimensions");

    std::vector<T> ret;
    if (this->transposed)
    {
        ret.resize(this->cols, T(0));
        for (std::size_t i = 0; i < this->rows; ++i)
            for (std::size_t j = this->row_ids[i]; j < this->row_ids[i + 1]; ++j)
                ret[this->col_index[j]]
                        += this->values[j] * rhs[this->col_index[j]];
    }
    else
    {
        ret.resize(this->rows, T(0));
        for (std::size_t i = 0; i < this->rows; ++i)
            for (std::size_t j = this->row_ids[i]; j < this->row_ids[i + 1]; ++j)
                ret[i] += this->values[j] * rhs[this->col_index[j]];
    }
    return ret;
}

template<typename T>
inline SparseMatrix<T>&
SparseMatrix<T>::mult_diagonal (T const& factor)
{
    for (std::size_t i = 0; i < this->rows; ++i)
        for (std::size_t j = this->row_ids[i]; j < this->row_ids[i + 1]; ++j)
            if (this->col_index[j] == i)
            {
                this->values[j] *= factor;
                break;
            }
            else if (this->col_index[j] > i)
            {
                break;
            }
    return *this;
}

template<typename T>
inline SparseMatrix<T>&
SparseMatrix<T>::transpose (void)
{
    this->transposed = !this->transposed;
    return *this;
}

MATH_NAMESPACE_END

#endif // MATH_SPARSE_MATRIX_HEADER

