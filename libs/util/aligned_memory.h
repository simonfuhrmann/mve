/*
 * Copyright (C) 2015, Benjamin Richter, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef UTIL_ALIGNED_MEMORY_HEADER
#define UTIL_ALIGNED_MEMORY_HEADER

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "util/defines.h"

UTIL_NAMESPACE_BEGIN

/**
 * Class that manages allocation and deletion of aligned heap memory.
 * The byte alignment size needs to be known at compile time.
 * The implementation allocates a little more memory (if necessary) and
 * shifts the aligned data pointer to correct in case the memory is not
 * already aligned. Copying the object will copy the memory.
 */
template <typename T, uintptr_t MODULO = 16>
class AlignedMemory
{
public:
    AlignedMemory (void);
    AlignedMemory (std::size_t size);
    AlignedMemory (AlignedMemory const& other);
    ~AlignedMemory (void);

    void allocate (std::size_t size);
    void deallocate (void);

    void operator= (AlignedMemory const& rhs);

    T const& operator[] (std::size_t index) const;
    T& operator[] (std::size_t index);

    T const& at (std::size_t index) const;
    T& at (std::size_t index);

    T const* begin (void) const;
    T* begin (void);
    T const* end (void) const;
    T* end (void);

private:
    unsigned char* raw;
    T* aligned;
    std::size_t size;
};

/* ---------------------------------------------------------------- */

template <typename T, uintptr_t MODULO>
inline
AlignedMemory<T, MODULO>::AlignedMemory (void)
    : raw(NULL), aligned(NULL), size(0)
{
}

template <typename T, uintptr_t MODULO>
inline
AlignedMemory<T, MODULO>::AlignedMemory (std::size_t size)
    : raw(NULL), aligned(NULL), size(0)
{
    this->allocate(size);
}

template <typename T, uintptr_t MODULO>
inline
AlignedMemory<T, MODULO>::AlignedMemory (AlignedMemory const& other)
    : raw(NULL), aligned(NULL), size(0)
{
    if (other.raw == NULL)
        return;
    this->allocate(other.size);
    std::copy(other.begin(), other.end(), this->aligned);
}

template <typename T, uintptr_t MODULO>
inline
AlignedMemory<T, MODULO>::~AlignedMemory (void)
{
    this->deallocate();
}

template <typename T, uintptr_t MODULO>
void
AlignedMemory<T, MODULO>::allocate (std::size_t size)
{
    this->deallocate();
    this->size = size;
    this->raw = new unsigned char[(size + MODULO - 1) * sizeof(T)];
    uintptr_t tmp = reinterpret_cast<uintptr_t>(this->raw);
    this->aligned = (tmp & ~(MODULO - 1)) == tmp
        ? reinterpret_cast<T*>(tmp)
        : reinterpret_cast<T*>((tmp + MODULO) & ~(MODULO - 1));
}

template <typename T, uintptr_t MODULO>
inline void
AlignedMemory<T, MODULO>::deallocate (void)
{
    delete [] this->raw;
    this->raw = NULL;
    this->aligned = NULL;
    this->size = 0;
}

template <typename T, uintptr_t MODULO>
inline void
AlignedMemory<T, MODULO>::operator= (AlignedMemory const& rhs)
{
    this->deallocate();
    if (rhs.raw == NULL)
        return;
    this->allocate(rhs.size);
    std::copy(rhs.begin(), rhs.end(), this->aligned);
}

template <typename T, uintptr_t MODULO>
inline T const&
AlignedMemory<T, MODULO>::operator[] (std::size_t index) const
{
    return this->aligned[index];
}

template <typename T, uintptr_t MODULO>
inline T&
AlignedMemory<T, MODULO>::operator[] (std::size_t index)
{
    return this->aligned[index];
}

template <typename T, uintptr_t MODULO>
inline T const&
AlignedMemory<T, MODULO>::at (std::size_t index) const
{
    return this->aligned[index];
}

template <typename T, uintptr_t MODULO>
inline T&
AlignedMemory<T, MODULO>::at (std::size_t index)
{
    return this->aligned[index];
}

template <typename T, uintptr_t MODULO>
inline T const*
AlignedMemory<T, MODULO>::begin (void) const
{
    return this->aligned;
}

template <typename T, uintptr_t MODULO>
inline T*
AlignedMemory<T, MODULO>::begin (void)
{
    return this->aligned;
}

template <typename T, uintptr_t MODULO>
inline T const*
AlignedMemory<T, MODULO>::end (void) const
{
    return this->aligned ? this->aligned + this->size : NULL;
}

template <typename T, uintptr_t MODULO>
inline T*
AlignedMemory<T, MODULO>::end (void)
{
    return this->aligned ? this->aligned + this->size : NULL;
}

UTIL_NAMESPACE_END

#endif /* UTIL_ALIGNED_MEMORY_HEADER */
