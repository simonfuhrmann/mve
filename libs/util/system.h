/*
 * Cross-plattform system utility functions.
 * Written by Simon Fuhrmann.
 */
#ifndef UTIL_SYSTEM_HEADER
#define UTIL_SYSTEM_HEADER

#include <ctime>
#include <cstdlib>

#include "util/defines.h"

#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#   define VC_EXTRALEAN
#   include <Windows.h>
#endif

UTIL_NAMESPACE_BEGIN
UTIL_SYSTEM_NAMESPACE_BEGIN

/*
 * ------------------------- Sleep functions--------------------------
 */

/** Sleeps the given amount of milli seconds. */
void sleep (std::size_t msec);
/** Sleeps the given amount of seconds. */
void sleep_sec (float secs);

/*
 * ------------------------- Random functions-------------------------
 */

/** Initializes the random number generator. */
void rand_init (void);
/** Initializes the random number generator with a given seed. */
void rand_seed (int seed);
/** Returns a floating point random number in [0, 1]. */
float rand_float (void);
/** Returns a random number in [0, 2^31]. */
int rand_int (void);

/*
 * ------------------------- Signal functions ------------------------
 */

/** Handles signal SIGSEGV (segmentation fault) printing a stack trace. */
void signal_segfault_handler (int code);

/** Prints a stack trace. */
void print_stack_trace (void);

/* ---------------------------------------------------------------- */

inline void
sleep (std::size_t msec)
{
#ifdef _WIN32
    Sleep((long)msec);
#else
    struct timespec rt, rm;
    rt.tv_sec = msec / 1000l;
    rt.tv_nsec = (msec - 1000l * rt.tv_sec) * 1000000l;
    ::nanosleep(&rt, &rm);
#endif
}

inline void
sleep_sec (float secs)
{
    sleep((std::size_t)(secs * 1000.0f));
}

inline void
rand_init (void)
{
    std::srand(std::time(NULL));
}

inline void
rand_seed (int seed)
{
    std::srand(seed);
}

inline float
rand_float (void)
{
    return (float)std::rand() / (float)RAND_MAX;
}

inline int
rand_int (void)
{
    return std::rand();
}

UTIL_SYSTEM_NAMESPACE_END
UTIL_NAMESPACE_END

#endif /* UTIL_SYSTEM_HEADER */
