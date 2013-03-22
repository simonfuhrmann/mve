/*
 * Aligned memory allocation.
 * Written by Simon Fuhrmann
 */

#ifndef UTIL_ALIGNED_MEMORY_HEADER
#define UTIL_ALIGNED_MEMORY_HEADER

#include <cstddef>

#include "util/defines.h"

UTIL_NAMESPACE_BEGIN

/**
 * Class that manages allocation and deletion of aligned heap memory.
 * The byte alignment size needs to be known at compile time.
 * The implementation allocates a little more memory and shifts the aligned
 * data pointer to correct in case the memory is not already aligned.
 */
template <typename T, uintptr_t MODULO = 16>
class AlignedMemory
{
public:
    AlignedMemory (void);
    AlignedMemory (std::size_t size);
    ~AlignedMemory (void);

    void allocate (std::size_t size);
    void deallocate (void);

    T const& operator[] (std::size_t index) const;
    T& operator[] (std::size_t index);

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
    : raw(0), aligned(0), size(0)
{
}

template <typename T, uintptr_t MODULO>
inline
AlignedMemory<T, MODULO>::AlignedMemory (std::size_t size)
    : raw(0), aligned(0), size(0)
{
    this->allocate(size);
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
    delete this->raw;
    this->raw = 0;
    this->aligned = 0;
    this->size = 0;
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
