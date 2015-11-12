/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef UTIL_SYSTEM_HEADER
#define UTIL_SYSTEM_HEADER

#include <ctime>
#include <cstdlib>
#include <thread>
#include <chrono>

#include "util/defines.h"

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
 * ---------------------- Signals / Application ----------------------
 */

/** Prints the application name and date and time of the build. */
void
print_build_timestamp (char const* application_name);

/** Prints the application name and the given date and time strings. */
void
print_build_timestamp (char const* application_name,
    char const* date, char const* time);

/** Registers signal SIGSEGV (segmentation fault) handler. */
void register_segfault_handler (void);

/** Handles signal SIGSEGV (segmentation fault) printing a stack trace. */
void signal_segfault_handler (int code);

/** Prints a stack trace. */
void print_stack_trace (void);

/* ---------------------------------------------------------------- */

inline void
sleep (std::size_t msec)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(msec));
}

inline void
sleep_sec (float secs)
{
    sleep((std::size_t)(secs * 1000.0f));
}

inline void
rand_init (void)
{
    std::srand(std::time(nullptr));
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

inline void
print_build_timestamp (char const* application_name)
{
    print_build_timestamp(application_name, __DATE__, __TIME__);
}

UTIL_SYSTEM_NAMESPACE_END
UTIL_NAMESPACE_END

#endif /* UTIL_SYSTEM_HEADER */
