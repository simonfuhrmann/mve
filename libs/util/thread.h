/*
 * A threading library wrapper around POSIX/Win32 threads.
 * Written by Simon Fuhrmann.
 * Win32 port written by Andre Schulz.
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

#include "defines.h"

UTIL_NAMESPACE_BEGIN

/**
 * Thread abstraction class for POSIX and Win32.
 */
class Thread
{
  private:
#ifdef _WIN32
    HANDLE handle;
    HANDLE cancel_event;
#elif defined(_POSIX_THREADS)
    pthread_t handle;
#endif
    bool cleanup;

    static void* stub (void* arg);

  protected:
    /**
     * This method needs to be defined in the subclass.
     * It is called when the thread is created using pt_create().
     * If transfered to public scope, run() may be called directly
     * for sequential execution (no threading involved then).
     */
    virtual void* run (void) = 0;

  public:
    Thread (void);
    virtual ~Thread (void);

    /** Creates a new thread and calls the run() method. */
    void pt_create (void);

    /** Sends a cancelation request to the thread. */
    void pt_cancel (void);

    /** Blocks the caller until the thread is terminated. */
    void pt_join (void);
};

/* ---------------------------------------------------------------- */

/**
 * Mutual exclusion thread lock abstraction for POSIX and Win32.
 * TODO: Win32 mutex abstraction not yet implemented.
 */
class Mutex
{
  private:
#ifdef _WIN32
    // TODO
#elif defined(_POSIX_THREADS)
    pthread_mutex_t mutex;
#endif

  private:
    /** Don't allow copying of the mutex. */
    Mutex (Mutex const& rhs);

    /** Don't allow copying of the mutex. */
    Mutex& operator= (Mutex const& rhs);

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
};

/* ---------------------------------------------------------------- */

/**
 * Semaphore thread lock abstraction for POSIX and Win32.
 */
class Semaphore
{
  private:
#ifdef _WIN32
    HANDLE sem;
    volatile LONG value;
#elif defined(_POSIX_THREADS)
    sem_t sem;
#endif

  private:
    /* Don't allow copying of semaphores. */
    Semaphore (Semaphore const& rhs);

    /* Don't allow copying of semaphores. */
    Semaphore& operator= (Semaphore const& rhs);

  public:
    /* Defaults to mutex. */
    Semaphore (unsigned int value = 1);
    Semaphore (unsigned int value, int pshared);
    ~Semaphore (void);

    /* DOWN the semaphore. */
    int wait (void);

    /* UP the semaphore. */
    int post (void);

    /* Returns the current value. */
    int get_value (void);
};

/* ---------------------------------------------------------------- */

/**
 * A read/write lock with shared read lock and exclusive write lock.
 * Several readers can access critical data but writing to the data
 * is exclusive, i.e. no other writer or reader is allowed during write.
 */
class ReadWriteLock
{
  private:
#ifdef _WIN32
    // TODO
#elif defined(_POSIX_THREADS)
    pthread_rwlock_t rwlock;
#endif

  private:
    /* Don't allow copying of the lock. */
    ReadWriteLock (ReadWriteLock const& rhs);

    /* Don't allow copying of the lock. */
    ReadWriteLock& operator= (ReadWriteLock const& rhs);

  public:
    ReadWriteLock (void);
    ~ReadWriteLock (void);

    int read_lock (void);
    int write_lock (void);
    int read_trylock (void);
    int write_trylock (void);
    int unlock (void);
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
    this->cancel_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    this->handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)
        Thread::stub, (void*)this, 0, NULL);

    if (this->handle == NULL)
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
    pthread_create(&this->handle, 0, Thread::stub, (void*)this);
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
    pthread_join(this->handle, 0);
}

#endif /* OS check */

/* ---------------------- Mutex implementation -------------------- */

#ifdef _WIN32

// TODO

#elif defined(_POSIX_THREADS)

inline
Mutex::Mutex (void)
{
    pthread_mutex_init(&this->mutex, 0);
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

inline Mutex&
Mutex::operator= (Mutex const& /*rhs*/)
{
    return *this;
}

inline
Mutex::Mutex (Mutex const& /*rhs*/)
{
}

#endif /* OS check */

/* -------------------- Semaphore implementation -------------------- */

inline
Semaphore::Semaphore (Semaphore const& /*rhs*/)
{
}

inline Semaphore&
Semaphore::operator= (Semaphore const& /*rhs*/)
{
    return *this;
}

#ifdef _WIN32

inline
Semaphore::Semaphore (unsigned int value)
{
    this->value = value;
    this->sem = CreateSemaphore(NULL, value, value, NULL);
}

/*
inline
Semaphore::Semaphore (unsigned int value, int pshared)
{
    sem_init(&sem, pshared, value);
}
*/

inline
Semaphore::~Semaphore (void)
{
    // TODO Destroy semaphore
}

inline int
Semaphore::wait (void)
{
    int retval = WaitForSingleObject(this->sem, INFINITE);
    if (retval == WAIT_OBJECT_0)
    {
        InterlockedDecrement(&this->value);
        return 0;
    }
    return -1;
}

inline int
Semaphore::post (void)
{
    InterlockedIncrement(&this->value);
    if (ReleaseSemaphore(this->sem, 1, NULL))
    {
        InterlockedDecrement(&this->value);
        return -1;
    }
    return 0;
}

inline int
Semaphore::get_value (void)
{
    return this->value;
}

#elif defined(_POSIX_THREADS)

inline
Semaphore::Semaphore (unsigned int value)
{
    sem_init(&sem, 0, value);
}

inline
Semaphore::Semaphore (unsigned int value, int pshared)
{
    sem_init(&sem, pshared, value);
}

inline
Semaphore::~Semaphore (void)
{
    sem_destroy(&sem);
}

inline int
Semaphore::wait (void)
{
    return sem_wait(&sem);
}

inline int
Semaphore::post (void)
{
    return sem_post(&sem);
}

inline int
Semaphore::get_value (void)
{
    int value;
    sem_getvalue(&sem, &value);
    return value;
}

#endif /* OS check */

/* ------------------Read-Write lock implementation --------------- */

inline
ReadWriteLock::ReadWriteLock (ReadWriteLock const& /*rhs*/)
{
}

inline ReadWriteLock&
ReadWriteLock::operator= (ReadWriteLock const& /*rhs*/)
{
    return *this;
}

#ifdef _WIN32

// TODO

#elif defined(_POSIX_THREADS)

inline
ReadWriteLock::ReadWriteLock (void)
{
    pthread_rwlock_init(&this->rwlock, 0);
}

inline
ReadWriteLock::~ReadWriteLock (void)
{
    pthread_rwlock_destroy(&this->rwlock);
}

inline int
ReadWriteLock::read_lock (void)
{
    return pthread_rwlock_rdlock(&this->rwlock);
}

inline int
ReadWriteLock::write_lock (void)
{
    return pthread_rwlock_wrlock(&this->rwlock);
}

inline int
ReadWriteLock::read_trylock (void)
{
    return pthread_rwlock_tryrdlock(&this->rwlock);
}

inline int
ReadWriteLock::write_trylock (void)
{
    return pthread_rwlock_trywrlock(&this->rwlock);
}

inline int
ReadWriteLock::unlock (void)
{
    return pthread_rwlock_unlock(&this->rwlock);
}

#endif /* OS check */

UTIL_NAMESPACE_END

#endif /* UTIL_THREAD_HEADER */
