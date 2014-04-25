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
  private:
    Mutex* m;

  public:
    MutexLock (Mutex& mutex)
    { this->m = &mutex; this->m->lock(); }

    ~MutexLock (void)
    { if (this->m) this->m->unlock(); }

    void unlock (void)
    { this->m->unlock(); this->m = NULL; }
};

/* ---------------------------------------------------------------- */

/**
 * Exception safe read lock for the reader/writer lock.
 */
class ReadLock
{
  private:
    ReadWriteLock* rwl;

  public:
    ReadLock (ReadWriteLock& rwlock)
    { this->rwl = &rwlock; this->rwl->read_lock(); }

    ~ReadLock (void)
    { if (this->rwl) this->rwl->unlock(); }

    void unlock (void)
    { this->rwl->unlock(); this->rwl = NULL; }
};

/* ---------------------------------------------------------------- */

/**
 * Exception safe write lock for the reader/writer lock.
 */
class WriteLock
{
  private:
    ReadWriteLock* rwl;

  public:
    WriteLock (ReadWriteLock& rwlock)
    { this->rwl = &rwlock; this->rwl->write_lock(); }

    ~WriteLock (void)
    { if (this->rwl) this->rwl->unlock(); }

    void unlock (void)
    { this->rwl->unlock(); this->rwl = NULL; }
};

UTIL_NAMESPACE_END

#endif /* UTIL_THREAD_LOCKS_HEADER */
