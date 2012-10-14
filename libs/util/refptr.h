/*
 * A reference counting smart pointer for C++
 * Written by Simon Fuhrmann.
 */

#ifndef UTIL_REFPTR_HEADER
#define UTIL_REFPTR_HEADER

#include <algorithm>

#include "defines.h"
#include "atomic.h"

UTIL_NAMESPACE_BEGIN

/**
 * The reference counter for the smart pointer.
 * The difficulty in the implementation of this class is thread safety,
 * which is ensured using atomic operations.
 */
class RefPtrCounter
{
public:
    RefPtrCounter (void);
    int increment (void);
    int decrement (void);
    int use_count (void) const;

private:
    Atomic<int> count;
};

/**
 * A reference counting smart pointer. Managed types are automatically
 * deleted if there are no more copies of the smart pointer around.
 */
template <class T>
class RefPtr
{
public:  // Constructors.
    /** Ctor that constructs a null pointer. */
    RefPtr (void);

    /** Ctor that takes ownership of 'p'. */
    explicit RefPtr (T* p);

    /** Copy constructor. */
    RefPtr (RefPtr<T> const& src);

    /** Ctor that takes ownership from different pointer type. */
    template <class Y>
    explicit RefPtr (Y* p);

    /** Copy constructor from different smart pointer. */
    template <class Y>
    RefPtr (RefPtr<Y> const& src);

    /** Destructor. */
    ~RefPtr (void);

public:  // Operations.
    /** Resets the pointer, possibly releasing resources. */
    void reset (void);

    /** Returns the use count (shared count) of the resource. */
    int use_count (void) const;

    /** Returns the pointer type. */
    T* get (void) const;

    /** Swaps this pointer with another pointer. */
    void swap (RefPtr<T>& p);

public:  // Operators.
    /** Dereference operator. */
    T& operator* (void) const;

    /** Member-access dereference operation. */
    T* operator-> (void) const;

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

    /** Comparison with bare pointer type. */
    bool operator== (T const* p) const;

    /** Comparison with other smart pointer. */
    bool operator== (RefPtr<T> const& p) const;

    /** Comparison with diffrent smart pointer. */
    template <class Y>
    bool operator== (RefPtr<Y> const& p) const;

    /** Comparison with bare pointer type. */
    bool operator!= (T const* p) const;

    /** Comparison with other smart pointer. */
    bool operator!= (RefPtr<T> const& p) const;

    /** Comparison with different smart pointer. */
    template <class Y>
    bool operator!= (RefPtr<Y> const& p) const;

    /** Less operation on pointer value for use in containers. */
    bool operator< (RefPtr<T> const& rhs);

    /** Less operation with different smart pointer. */
    // FIXME: Causes some weired ambiguities.
    //template <class Y>
    //bool operator< (RefPtr<Y> const& rhs) const;

protected:
    /* Data pointer and counter. */
    T* ptr;
    RefPtrCounter* counter;

private:
    /* Template friends for accessing diffrent RefPtr's. */
    template <class Y> friend class RefPtr;
    void increment (void);
    void decrement (void);
};

/* ---------------------------------------------------------------- */

inline
RefPtrCounter::RefPtrCounter (void)
    : count(1)
{
}

inline int
RefPtrCounter::increment (void)
{
    return count.increment();
}

inline int
RefPtrCounter::decrement (void)
{
    return count.decrement();
}

inline int
RefPtrCounter::use_count (void) const
{
    return *count;
}

/* ---------------------------------------------------------------- */

template <class T>
inline
RefPtr<T>::RefPtr (void)
    : ptr(0)
    , counter(0)
{
}

template <class T>
inline
RefPtr<T>::RefPtr (T* p) : ptr(p)
{
    counter = (p == 0) ? 0 : new RefPtrCounter();
}

template <class T>
inline
RefPtr<T>::RefPtr (RefPtr<T> const& src)
    : ptr(src.ptr)
    , counter(src.counter)
{
    increment();
}

template <class T>
template <class Y>
inline
RefPtr<T>::RefPtr (Y* p) : ptr(static_cast<T*>(p))
{
    counter = (p == 0) ? 0 : new RefPtrCounter();
}

template <class T>
template <class Y>
inline
RefPtr<T>::RefPtr (RefPtr<Y> const& src)
    : ptr(static_cast<T*>(src.ptr))
    , counter(src.counter)
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
inline void
RefPtr<T>::reset (void)
{
    decrement();
    ptr = 0;
    counter = 0;
}

template <class T>
inline int
RefPtr<T>::use_count (void) const
{
    return ptr == 0 ? 0 : counter->use_count();
}

template <class T>
inline T*
RefPtr<T>::get (void) const
{
    return ptr;
}

template <class T>
inline void
RefPtr<T>::swap (RefPtr<T>& p)
{
    std::swap(p.ptr, ptr);
    std::swap(p.counter, counter);
}

template <class T>
/*inline*/ void
RefPtr<T>::increment (void)
{
    if (ptr == 0)
        return;
    counter->increment();
}

template <class T>
/*inline*/ void
RefPtr<T>::decrement (void)
{
    if (ptr == 0)
        return;
    if (counter->decrement() == 0)
    {
        delete counter;
        delete ptr;
    }
}

template <class T>
inline T&
RefPtr<T>::operator* (void) const
{
    return *ptr;
}

template <class T>
inline T*
RefPtr<T>::operator-> (void) const
{
    return ptr;
}

template <class T>
inline RefPtr<T>&
RefPtr<T>::operator= (RefPtr<T> const& rhs)
{
    if (rhs.ptr == ptr)
        return *this;
    decrement();
    ptr = rhs.ptr;
    counter = rhs.counter;
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
    counter = (ptr == 0) ? 0 : new RefPtrCounter();
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
    counter = rhs.counter;
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
    counter = (ptr == 0) ? 0 : new RefPtrCounter();
    return *this;
}

template <class T>
inline bool
RefPtr<T>::operator== (T const* p) const
{
    return p == ptr;
}

template <class T>
inline bool
RefPtr<T>::operator== (RefPtr<T> const& p) const
{
    return p.ptr == ptr;
}

template <class T>
template <class Y>
inline bool
RefPtr<T>::operator== (RefPtr<Y> const& p) const
{
    return p.ptr == ptr;
}

template <class T>
inline bool
RefPtr<T>::operator!= (T const* p) const
{
    return p != ptr;
}

template <class T>
inline bool
RefPtr<T>::operator!= (RefPtr<T> const& p) const
{
    return p.ptr != ptr;
}

template <class T>
template <class Y>
inline bool
RefPtr<T>::operator!= (RefPtr<Y> const& p) const
{
    return p.ptr != ptr;
}

template <class T>
inline bool
RefPtr<T>::operator< (RefPtr<T> const& rhs)
{
    return ptr < rhs.ptr;
}

//template <class T>
//template <class Y>
//inline bool
//RefPtr<T>::operator< (RefPtr<Y> const& rhs) const
//{
//    return ptr < rhs.ptr;
//}

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

#endif /* UTIL_REFPTR_HEADER */
