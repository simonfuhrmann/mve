/*
 * Class that takes CPU time and provides elapsed times.
 * Written by Simon Fuhrmann.
 */
#ifndef UTIL_CLOCK_TIMER_HEADER
#define UTIL_CLOCK_TIMER_HEADER

#include <ctime>

#include "defines.h"

UTIL_NAMESPACE_BEGIN

/**
 * Very simple timer class to take execution times. The reported
 * float values are in seconds, the integer values are in milli seconds.
 * The functions that provide milli seconds should be preferred.
 *
 * This class should not be used for timings that rely on the
 * actual real world time but rather for computational timings.
 * The timings here are pure processing times which vary from
 * real time if the application is scheduled (e.g. when using I/O)
 * or sleeps or uses several threads.
 */
class ClockTimer
{
private:
    std::size_t start;

public:
    ClockTimer (void);
    void reset (void);

    static float now_sec (void);
    static std::size_t now (void);

    std::size_t get_elapsed (void) const;
    float get_elapsed_sec (void) const;
};

/* ---------------------------------------------------------------- */

inline
ClockTimer::ClockTimer (void)
{
    this->reset();
}

inline void
ClockTimer::reset (void)
{
    this->start = ClockTimer::now();
}

inline float
ClockTimer::now_sec (void)
{
    return (float)std::clock() / (float)CLOCKS_PER_SEC;
}

inline std::size_t
ClockTimer::now (void)
{
    return ((std::size_t)(std::clock()) * 1000) / (std::size_t)CLOCKS_PER_SEC;
}

inline float
ClockTimer::get_elapsed_sec (void) const
{
    return (1.0f / 1000.0f) * (float)this->get_elapsed();
}

inline std::size_t
ClockTimer::get_elapsed (void) const
{
    return ClockTimer::now() - start;
}

UTIL_NAMESPACE_END

#endif /* UTIL_CLOCK_TIMER_HEADER */
