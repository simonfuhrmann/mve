/*
 * A reference counting smart pointer for C++
 * Written by Simon Fuhrmann.
 */

#ifndef UTIL_REFPTR_HEADER
#define UTIL_REFPTR_HEADER

/* If defined, the SP uses thread unsafe inc and dec operations. */
#define UTIL_REFPTR_UNSAFE 0
/* If defined, the SP uses atomic inc and dec operations. */
#define UTIL_REFPTR_ATOMIC 1

#include <algorithm>

#include "defines.h"
#if UTIL_REFPTR_ATOMIC
#   include "atomic.h"
#endif

UTIL_NAMESPACE_BEGIN

/**
 * A reference counting smart pointer. Managed types are automatically
 * deleted if there are no more copies of the smart pointer around.
 */
template <class T>
class RefPtr
{
private:
    /* Template friends for accessing diffrent RefPtr's. */
    template <class Y> friend class RefPtr;

    /* Defines the counter type to use. */
#if UTIL_REFPTR_UNSAFE
    typedef int CounterType;
#elif UTIL_REFPTR_ATOMIC
    typedef Atomic<int> CounterType;
#endif

protected:
    /* Data pointer and counter. */
    T* ptr;
    CounterType* count;

/* Private definition of member methods. */
private:
    void increment (void);
    void decrement (void);

/* Public definition of member methods. */
public:
    /** Ctor that constructs a null pointer. */
    RefPtr (void);

    /** Ctor that takes ownership of 'p'. */
    explicit RefPtr (T* p);

    /** Copy constructor. */
    RefPtr (RefPtr<T> const& src);

    /** Destructor. */
    ~RefPtr (void);

    /** Ctor that takes ownership from different pointer type. */
    template <class Y>
    explicit RefPtr (Y* p);

    /** Copy constructor from different smart pointer. */
    template <class Y>
    RefPtr (RefPtr<Y> const& src);

    /** Assignment from other smart pointer. */
    RefPtr<T>& operator= (RefPtr<T> const& rhs);

    /** Assignment that takes ownership of 'rhs'. */
    RefPtr<T>& operator= (T* rhs);

    /** Assignment from different RefPtr. */
    template <class Y>
    RefPtr<T>& operator= (RefPtr<Y> const& rhs);

    /** Assignment from different pointer. */
    template <class Y>
    RefPtr<T>& operator= (Y* rhs);

public:
    /** Resets the pointer, possibly releasing resources. */
    void reset (void);

    /** Swaps this pointer with another pointer. */
    void swap (RefPtr<T>& p);

public:
    /** Dereferences pointer. */
    T& operator* (void) const
    { return *ptr; }

    /** Member-access dereference operation. */
    T* operator-> (void) const
    { return ptr; }

    /** Comparison with bare pointer type. */
    bool operator== (T const* p) const
    { return p == ptr; }

    /** Comparison with other smart pointer. */
    bool operator== (RefPtr<T> const& p) const
    { return p.ptr == ptr; }

    /** Comparison with bare pointer type. */
    bool operator!= (T const* p) const
    { return p != ptr; }

    /** Comparison with other smart pointer. */
    bool operator!= (RefPtr<T> const& p) const
    { return p.ptr != ptr; }

    /** Less operation on pointer value for use in containers. */
    bool operator< (RefPtr<T> const& rhs)
    { return ptr < rhs.ptr; }

    /** Returns the use count (shared count) of the resource. */
    int use_count (void) const;

    /** Returns the pointer type. */
    T* get (void) const
    { return ptr; }

    /** Comparison with diffrent smart pointer. */
    template <class Y>
    bool operator== (RefPtr<Y> const& p) const
    { return p.ptr == ptr; }

    /** Comparison with different smart pointer. */
    template <class Y>
    bool operator!= (RefPtr<Y> const& p) const
    { return p.ptr != ptr; }

    /** Less operation with different smart pointer. */
    template <class Y>
    bool operator< (RefPtr<Y> const& rhs) const
    { return ptr < rhs.ptr; }
};

UTIL_NAMESPACE_END

/* ---------------------------------------------------------------- */

STD_NAMESPACE_BEGIN

/** Specialization of std::swap for efficient smart pointer swapping. */
template <class T>
inline void
swap (util::RefPtr<T>& a, util::RefPtr<T>& b)
{
    a.swap(b);
}

STD_NAMESPACE_END

/* ---------------------------------------------------------------- */

UTIL_NAMESPACE_BEGIN

template <class T>
inline
RefPtr<T>::RefPtr (void)
    : ptr(0)
    , count(0)
{
}

template <class T>
inline
RefPtr<T>::RefPtr (T* p) : ptr(p)
{
    count = (p == 0) ? 0 : new CounterType(1);
}

template <class T>
inline
RefPtr<T>::RefPtr (RefPtr<T> const& src)
    : ptr(src.ptr)
    , count(src.count)
{
    increment();
}

template <class T>
inline
RefPtr<T>::~RefPtr (void)
{
    decrement();
}

template <class T>
template <class Y>
inline
RefPtr<T>::RefPtr (Y* p) : ptr(static_cast<T*>(p))
{
    count = (p == 0) ? 0 : new CounterType(1);
}

template <class T>
template <class Y>
inline
RefPtr<T>::RefPtr (RefPtr<Y> const& src)
    : ptr(static_cast<T*>(src.ptr))
    , count(src.count)
{
    increment();
}

template <class T>
inline RefPtr<T>&
RefPtr<T>::operator= (RefPtr<T> const& rhs)
{
    if (rhs.ptr == ptr)
        return *this;
    decrement();
    ptr = rhs.ptr;
    count = rhs.count;
    increment();
    return *this;
}

template <class T>
inline RefPtr<T>&
RefPtr<T>::operator= (T* rhs)
{
    if (rhs == ptr)
        return *this;
    decrement();
    ptr = rhs;
    count = (ptr == 0) ? 0 : new CounterType(1);
    return *this;
}

template <class T>
template <class Y>
inline RefPtr<T>&
RefPtr<T>::operator= (RefPtr<Y> const& rhs)
{
    if (rhs.ptr == ptr)
        return *this;
    decrement();
    ptr = static_cast<T*>(rhs.ptr);
    count = rhs.count;
    increment();
    return *this;
}

template <class T>
template <class Y>
inline RefPtr<T>&
RefPtr<T>::operator= (Y* rhs)
{
    if (rhs == ptr)
        return *this;
    decrement();
    ptr = static_cast<T*>(rhs);
    count = (ptr == 0) ? 0 : new CounterType(1);
    return *this;
}


template <class T>
inline void
RefPtr<T>::reset (void)
{
    decrement();
    ptr = 0;
    count = 0;
}

template <class T>
inline void
RefPtr<T>::swap (RefPtr<T>& p)
{
    std::swap(p.ptr, ptr);
    std::swap(p.count, count);
}

template <class T>
/*inline*/ void
RefPtr<T>::increment (void)
{
    if (ptr == 0)
        return;
#if UTIL_REFPTR_UNSAFE
    ++(*count);
#elif UTIL_REFPTR_ATOMIC
    count->increment();
#endif
}

template <class T>
/*inline*/ void
RefPtr<T>::decrement (void)
{
    if (ptr == 0)
        return;
#if UTIL_REFPTR_UNSAFE
    if (--(*count) == 0)
#elif UTIL_REFPTR_ATOMIC
    if (count->decrement() == 0)
#endif
    {
        delete count;
        delete ptr;
    }
}

template <class T>
inline int
RefPtr<T>::use_count (void) const
{
#if UTIL_REFPTR_UNSAFE
    return count == 0 ? 0 : *count;
#elif UTIL_REFPTR_ATOMIC
    return count == 0 ? 0 : **count;
#endif
}

UTIL_NAMESPACE_END

#endif /* UTIL_REFPTR_HEADER */
