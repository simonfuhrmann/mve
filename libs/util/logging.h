/*
 * A simple logging framework that is typesafe, threadsafe,
 * efficient, portable, fine-grained, compact, and flexible.
 *
 * Rewritten by Simon Fuhrmann from Article at
 * http://www.drdobbs.com/cpp/201804215
 */

#ifndef UTIL_LOGGING_HEADER
#define UTIL_LOGGING_HEADER

#include <sstream>

#include "defines.h"

UTIL_NAMESPACE_BEGIN
#if 0
template <typename P, typename F>
class Logging
{
protected:
    std::ostringstream os;

public:
    Logging (void);
    virtual ~Logging (void);

    std::ostringstream& get (void);
};

/* ---------------------------------------------------------------- */

template <typename P, typename F>
Logging<P,F>::Logging (void)
{
}

template <typename P, typename F>
Logging<P,F>::~Logging (void)
{
    P::output(F::format(os.str().c_str()));
}

template <typename P, typename F>
Logging<P,F>::get (void)
{
    return this->os;
}
#endif
UTIL_NAMESPACE_END

#endif /* UTIL_LOGGING_HEADER */
