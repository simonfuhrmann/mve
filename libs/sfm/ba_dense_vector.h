/*
 * Copyright (C) 2015, Simon Fuhrmann, Fabian Langguth
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef SFM_BA_DENSE_VECTOR_HEADER
#define SFM_BA_DENSE_VECTOR_HEADER

#include <cmath>
#include <stdexcept>
#include <vector>

#include "sfm/defines.h"

SFM_NAMESPACE_BEGIN
SFM_BA_NAMESPACE_BEGIN

template <typename T>
class DenseVector
{
public:
    DenseVector (void) = default;
    DenseVector (std::size_t size, T const& value = T(0));
    void resize (std::size_t size, T const& value = T(0));
    void clear (void);
    void fill (T const& value);
    std::size_t size (void) const;

    T* data (void);
    T const* data (void) const;
    T* begin (void);
    T const* begin (void) const;
    T* end (void);
    T const* end (void) const;

    DenseVector operator- (void) const;
    bool operator== (DenseVector const& rhs) const;
    T& operator[] (std::size_t index);
    T const& operator[] (std::size_t index) const;
    T& at (std::size_t index);
    T const& at (std::size_t index) const;

    T norm (void) const;
    T squared_norm (void) const;
    T dot (DenseVector const& rhs) const;
    DenseVector add (DenseVector const& rhs) const;
    DenseVector subtract (DenseVector const& rhs) const;
    DenseVector multiply (T const& factor) const;
    void multiply_self (T const& factor);
    void negate_self (void);

private:
    std::vector<T> values;
};

/* ------------------------ Implementation ------------------------ */

template <typename T>
inline
DenseVector<T>::DenseVector (std::size_t size, T const& value)
{
    this->resize(size, value);
}

template <typename T>
inline void
DenseVector<T>::resize (std::size_t size, T const& value)
{
    this->values.clear();
    this->values.resize(size, value);
}

template <typename T>
inline void
DenseVector<T>::clear (void)
{
    this->values.clear();
}

template <typename T>
inline void
DenseVector<T>::fill (T const& value)
{
    std::fill(this->values.begin(), this->values.end(), value);
}

template <typename T>
std::size_t
DenseVector<T>::size (void) const
{
    return this->values.size();
}

template <typename T>
T*
DenseVector<T>::data (void)
{
    return this->values.data();
}

template <typename T>
T const*
DenseVector<T>::data (void) const
{
    return this->values.data();
}

template <typename T>
T*
DenseVector<T>::begin (void)
{
    return this->values.data();
}

template <typename T>
T const*
DenseVector<T>::begin (void) const
{
    return this->values.data();
}

template <typename T>
T*
DenseVector<T>::end (void)
{
    return this->values.data() + this->values.size();
}

template <typename T>
T const*
DenseVector<T>::end (void) const
{
    return this->values.data() + this->values.size();
}

template <typename T>
DenseVector<T>
DenseVector<T>::operator- (void) const
{
    DenseVector ret(this->size());
    for (std::size_t i = 0; i < this->size(); ++i)
        ret[i] = -this->at(i);
    return ret;
}

template <typename T>
bool
DenseVector<T>::operator== (DenseVector const& rhs) const
{
    if (this->size() != rhs.size())
        return false;
    for (std::size_t i = 0; i < this->size(); ++i)
        if (this->at(i) != rhs.at(i))
            return false;
    return true;
}

template <typename T>
T&
DenseVector<T>::operator[] (std::size_t index)
{
    return this->values[index];
}

template <typename T>
T const&
DenseVector<T>::operator[] (std::size_t index) const
{
    return this->values[index];
}

template <typename T>
T&
DenseVector<T>::at (std::size_t index)
{
    return this->values[index];
}

template <typename T>
T const&
DenseVector<T>::at (std::size_t index) const
{
    return this->values[index];
}

template <typename T>
inline T
DenseVector<T>::norm (void) const
{
    return std::sqrt(this->squared_norm());
}

template <typename T>
T
DenseVector<T>::squared_norm (void) const
{
    return this->dot(*this);
}

template <typename T>
T
DenseVector<T>::dot (DenseVector<T> const& rhs) const
{
    if (this->size() != rhs.size())
        throw std::invalid_argument("Incompatible vector dimensions");

    T ret(0);
    for (std::size_t i = 0; i < this->size(); ++i)
        ret += this->values[i] * rhs.values[i];
    return ret;
}

template <typename T>
DenseVector<T>
DenseVector<T>::subtract (DenseVector<T> const& rhs) const
{
    if (this->size() != rhs.size())
        throw std::invalid_argument("Incompatible vector dimensions");

    DenseVector<T> ret(this->size(), T(0));
    for (std::size_t i = 0; i < this->size(); ++i)
        ret.values[i] = this->values[i] - rhs.values[i];
    return ret;
}

template <typename T>
DenseVector<T>
DenseVector<T>::add (DenseVector<T> const& rhs) const
{
    if (this->size() != rhs.size())
        throw std::invalid_argument("Incompatible vector dimensions");

    DenseVector<T> ret(this->size(), T(0));
    for (std::size_t i = 0; i < this->size(); ++i)
        ret.values[i] = this->values[i] + rhs.values[i];
    return ret;
}

template <typename T>
DenseVector<T>
DenseVector<T>::multiply (T const& factor) const
{
    DenseVector<T> ret(this->size(), T(0));
    for (std::size_t i = 0; i < this->size(); ++i)
        ret[i] = this->at(i) * factor;
    return ret;
}

template <typename T>
void
DenseVector<T>::multiply_self (T const& factor)
{
    for (std::size_t i = 0; i < this->size(); ++i)
        this->at(i) *= factor;
}

template <typename T>
void
DenseVector<T>::negate_self (void)
{
    for (std::size_t i = 0; i < this->size(); ++i)
        this->at(i) = -this->at(i);
}

SFM_BA_NAMESPACE_END
SFM_NAMESPACE_END

#endif // SFM_BA_DENSE_VECTOR_HEADER
