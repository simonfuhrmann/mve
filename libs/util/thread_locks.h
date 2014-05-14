/*
 * A threading library wrapper around POSIX/Win32 threads.
 * Written by Simon Fuhrmann.
 *
 * Thread locks provide an exception safe way to create locks. A lock is
 * established on as soon as the lock object is created and destroyed when
 * unlock() or the destructor is called, whichever happens first.
 */

#ifndef UTIL_THREAD_LOCKS_HEADER
#define UTIL_THREAD_LOCKS_HEADER

#include "util/defines.h"
#include "util/thread.h"

UTIL_NAMESPACE_BEGIN

/**
 * Exception safe lock for the mutex.
 */
class MutexLock
{
public:
    MutexLock (Mutex& mutex)
    { this->m = &mutex; this->m->lock(); }

    ~MutexLock (void)
    { if (this->m) this->m->unlock(); }

    void unlock (void)
    { this->m->unlock(); this->m = NULL; }

private:
    Mutex* m;
};

UTIL_NAMESPACE_END

#endif /* UTIL_THREAD_LOCKS_HEADER */
