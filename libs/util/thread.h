/*
 * Copyright (C) 2015, Simon Fuhrmann, Andre Schulz
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef UTIL_THREAD_HEADER
#define UTIL_THREAD_HEADER

#ifndef _WIN32
#   include <unistd.h>
#endif

#ifdef _WIN32
#   include <iostream>
#   define WIN32_LEAN_AND_MEAN
#   define VC_EXTRALEAN
#   define NOMINMAX
#   include <windows.h>
#elif defined(_POSIX_THREADS)
#   include <pthread.h>
#   include <semaphore.h>
#else
#   error "No thread abstraction available."
#endif

#include "util/defines.h"

UTIL_NAMESPACE_BEGIN

/**
 * Thread abstraction class for POSIX and Win32.
 */
class Thread
{
public:
    Thread (void);
    virtual ~Thread (void);

    /** Creates a new thread and calls the run() method. */
    void pt_create (void);

    /** Sends a cancelation request to the thread. */
    void pt_cancel (void);

    /** Blocks the caller until the thread is terminated. */
    void pt_join (void);

protected:
    /**
     * This method needs to be defined in the subclass.
     * It is called when the thread is created using pt_create().
     * If transfered to public scope, run() may be called directly
     * for sequential execution (no threading involved then).
     */
    virtual void* run (void) = 0;

private:
#ifdef _WIN32
    HANDLE handle;
    HANDLE cancel_event;
#elif defined(_POSIX_THREADS)
    pthread_t handle;
#endif
    bool cleanup;

    static void* stub (void* arg);
};

/* ---------------------------------------------------------------- */

/**
 * Mutual exclusion thread lock abstraction for POSIX and Win32.
 */
class Mutex
{
public:
    Mutex (void);
    ~Mutex (void);

    /**
     * Acquire ownership of the mutex. This will block if the mutex is
     * already locked (by another thread) until the mutex is released.
     */
    int lock (void);

    /**
     * This call acquires ownership of the mutex. If the mutex is
     * already locked, this call will NOT block and return with EBUSY.
     */
    int trylock (void);

    /**
     * Release ownership of the mutex, which unlocks the mutex.
     */
    int unlock (void);

private:
    /** Don't allow copying of the mutex. */
    Mutex (Mutex const& rhs);

    /** Don't allow copying of the mutex. */
    Mutex& operator= (Mutex const& rhs);

private:
#ifdef _WIN32
    /* On Windows there are two ways for mutual exclusion: Critical sections
     * and Mutexes. Critical sections only work for mutual exclusion for
     * threads which are in the same process. Mutexes also work across
     * different processes.
     *
     * Pthreads mutexes can work for both single and multiple processes but
     * defaults to single process. (see pthread_mutexattr_{get,set}pshared())
     * According to MSDN, critical section objects are slightly faster and
     * more efficient than mutex objects. I'm assuming that using mutexes
     * across processes is rare and thus prefer the slightly more efficient
     * critical section over the mutex object.
     * (see http://msdn.microsoft.com/en-us/library/ms682530%28v=vs.85%29.aspx)
     */
    CRITICAL_SECTION mutex;
#elif defined(_POSIX_THREADS)
    pthread_mutex_t mutex;
#endif
};

/* --------------------- Thread implementation -------------------- */

inline
Thread::Thread (void)
{
    this->cleanup = false;
}

inline void*
Thread::stub (void* arg)
{
    return ((Thread*)arg)->run();
}

#ifdef _WIN32

inline
Thread::~Thread (void)
{
    if (this->cleanup)
    {
        CloseHandle(this->cancel_event);
        CloseHandle(this->handle);
    }
}

inline void
Thread::pt_create (void)
{
    this->cleanup = true;
    this->cancel_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    this->handle = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)
        Thread::stub, (void*)this, 0, nullptr);

    if (this->handle == nullptr)
      std::cerr << "ERROR: CreateThread(): " << GetLastError() << std::endl;
}

inline void
Thread::pt_cancel (void)
{
    SetEvent(this->cancel_event);
}

inline void
Thread::pt_join (void)
{
    this->cleanup = false;
    WaitForSingleObject(this->handle, INFINITE);
    CloseHandle(this->cancel_event);
    CloseHandle(this->handle);
}

#elif defined(_POSIX_THREADS)

inline
Thread::~Thread (void)
{
    if (this->cleanup)
        pthread_detach(this->handle);
}

inline void
Thread::pt_create (void)
//Thread::pt_create (pthread_attr_t const* p = 0)
{
    this->cleanup = true;
    pthread_create(&this->handle, nullptr, Thread::stub, (void*)this);
    //pthread_create(&this->handle, p, Thread::stub, (void*)this);
}

inline void
Thread::pt_cancel (void)
{
    pthread_cancel(this->handle);
}

inline void
Thread::pt_join (void)
{
    this->cleanup = false;
    pthread_join(this->handle, nullptr);
}

#endif /* OS check */

/* ---------------------- Mutex implementation -------------------- */

#ifdef _WIN32

inline
Mutex::Mutex (void)
{
    InitializeCriticalSection(&this->mutex);
}

inline
Mutex::~Mutex (void)
{
    DeleteCriticalSection(&this->mutex);
}

inline int
Mutex::lock (void)
{
    EnterCriticalSection(&this->mutex);
    return 0;
}

inline int
Mutex::trylock (void)
{
    /*
     * retval is nonzero on success, zero otherwise. pthread_mutex_trylock()
     * has opposite return values. Thus, we transform the return value to
     * match pthreads.
     */
    BOOL retval = TryEnterCriticalSection(&this->mutex);
    return (retval == 0);
}

inline int
Mutex::unlock (void)
{
    LeaveCriticalSection(&this->mutex);
    return 0;
}

#elif defined(_POSIX_THREADS)

inline
Mutex::Mutex (void)
{
    pthread_mutex_init(&this->mutex, nullptr);
}

inline
Mutex::~Mutex (void)
{
    pthread_mutex_destroy(&this->mutex);
}

inline int
Mutex::lock (void)
{
    return pthread_mutex_lock(&this->mutex);
}

inline int
Mutex::trylock (void)
{
    return pthread_mutex_trylock(&this->mutex);
}

inline int
Mutex::unlock (void)
{
    return pthread_mutex_unlock(&this->mutex);
}

#endif /* OS check */

UTIL_NAMESPACE_END

#endif /* UTIL_THREAD_HEADER */
