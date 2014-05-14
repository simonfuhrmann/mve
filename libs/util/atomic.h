/*
 * Atomic operations abstraction.
 * Written by Simon Fuhrmann.
 *
 * Note: Windows implementatoin only supports 32bit int types.
 */

#ifndef UTIL_ATOMIC_HEADER
#define UTIL_ATOMIC_HEADER

#ifdef _WIN32
#   define NOMINMAX
#   include "windows.h"
#endif

#include "util/defines.h"

UTIL_NAMESPACE_BEGIN

/**
 * Type wrapper that performs atomic operations.
 * Currently, only increment and decrement are supported.
 *
 * On Linux, implementation relies on the GCC atomic operations buildin
 * functions. GCC will allow any integral scalar or pointer type that
 * is 1, 2, 4 or 8 bytes in length.
 *
 * On Windows, the implementation uses MSDN synchronization functions,
 * currently only 32 bit integer types are supported.
 * http://msdn.microsoft.com/en-us/library/ms686360(v=vs.85).aspx
 *
 * Docs: http://gcc.gnu.org/onlinedocs/gcc-4.4.1/gcc/Atomic-Builtins.html
 */
template <typename T>
class Atomic
{
public:
    /** Creates an atomic variable with initial value 'init'. */
    Atomic (T const& init);

    /** Atomically increments the variable. */
    T increment (void);
    /** Atomically decrements the variable. */
    T decrement (void);

    /**
     * Increments the atomic variable only if the value is zero.
     * The call is blocking as long as the variable is not equal to zero.
     * WARNING: The blocking is implemented using active waiting.
     */
    void mutex_up (void);

    /**
     * Decrements the atomic variable only if the value is one.
     * The call is blocking as long as the variable is not equal to one.
     * WARNING: The blocking is implemented using active waiting.
     */
    void mutex_down (void);

    /** Returns the value. */
    volatile T const& operator* (void) const;

private:
    volatile T val;
};

/* ---------------------------------------------------------------- */

/**
 * Exception-safe atomic mutex lock. The lock is aquired on construction
 * of the object, and the lock is released if 'release()' is called or on
 * destruction of the object, whichever occures first.
 */
template <typename T>
class AtomicMutex
{
public:
    AtomicMutex (Atomic<T>& atomic);
    ~AtomicMutex (void);
    void release (void);

private:
    Atomic<T>* lock;
};

/* ---------------------------------------------------------------- */

template <typename T>
inline
Atomic<T>::Atomic (T const& init)
    : val(init)
{
}

template <typename T>
inline T
Atomic<T>::increment (void)
{
#ifdef _WIN32
    return InterlockedIncrement(&this->val);
#else
    return __sync_add_and_fetch(&this->val, 1);
#endif
}

template <typename T>
inline T
Atomic<T>::decrement (void)
{
#ifdef _WIN32
    return InterlockedDecrement(&this->val);
#else
    return __sync_sub_and_fetch(&this->val, 1);
#endif
}

template <typename T>
inline void
Atomic<T>::mutex_up (void)
{
#ifdef _WIN32
    while (InterlockedCompareExchange((LONG volatile*)&this->val, 1, 0) != 0)
    {
    }
#else
    while (true)
    {
        if (__sync_bool_compare_and_swap(&this->val, 0, 1))
            return;
    }
#endif
}

template <typename T>
inline void
Atomic<T>::mutex_down (void)
{
#ifdef _WIN32
    while (InterlockedCompareExchange((LONG volatile*)&this->val, 0, 1) != 1)
    {
    }
#else
    while (true)
    {
        if (__sync_bool_compare_and_swap(&this->val, 1, 0))
            return;
    }
#endif
}

template <typename T>
inline volatile T const&
Atomic<T>::operator* (void) const
{
    return this->val;
}

template <typename T>
inline
AtomicMutex<T>::AtomicMutex (Atomic<T>& atomic)
{
    this->lock = &atomic;
    this->lock->mutex_up();
}

template <typename T>
inline
AtomicMutex<T>::~AtomicMutex (void)
{
    this->release();
}

template <typename T>
inline void
AtomicMutex<T>::release (void)
{
    if (!this->lock)
        return;
    this->lock->mutex_down();
    this->lock = NULL;
}

UTIL_NAMESPACE_END

#endif /* UTIL_ATOMIC_HEADER */
