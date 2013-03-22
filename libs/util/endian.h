/*
 * Endian conversion functions.
 * Written by Simon Fuhrmann, Andre Schulz.
 */

#ifndef UTIL_ENDIAN_HEADER
#define UTIL_ENDIAN_HEADER

#include <algorithm>

#include "util/defines.h"

UTIL_NAMESPACE_BEGIN
UTIL_SYSTEM_NAMESPACE_BEGIN

/** Swaps little/big endianess of the operand. */
template <int N>
inline void
byte_swap (char* data);

/** Little endian to host order conversion. */
template <typename T>
inline T
letoh (T const& x);

/** Big endian to host order conversion. */
template <typename T>
inline T
betoh (T const& x);

/* ---------------------------------------------------------------- */

template <>
inline void
byte_swap<2> (char* data)
{
    std::swap(data[0], data[1]);
}

template <>
inline void
byte_swap<4> (char* data)
{
    std::swap(data[0], data[3]);
    std::swap(data[1], data[2]);
}

template <>
inline void
byte_swap<8> (char* data)
{
    std::swap(data[0], data[7]);
    std::swap(data[1], data[6]);
    std::swap(data[2], data[5]);
    std::swap(data[3], data[4]);
}

/* Determine host byte-order.
 *
 * "All versions of windows run little-endian, period."
 * http://social.msdn.microsoft.com/Forums/en-US/windowsmobiledev/thread/04c92ef9-e38e-415f-8958-ec9f7c196fd3
 */
#if defined(_WIN32)
#   define HOST_BYTEORDER_LE
#endif

#if defined(__APPLE__)
#   if defined(__ppc__) || defined(__ppc64__)
#       define HOST_BYTEORDER_BE
#   elif defined(__i386__) || defined(__x86_64__)
#       define HOST_BYTEORDER_LE
#   endif
#endif

#if defined(__linux__)
#   include <endian.h> // part of glibc 2.9
#   if __BYTE_ORDER == __LITTLE_ENDIAN
#       define HOST_BYTEORDER_LE
#   else
#       define HOST_BYTEORDER_BE
#   endif
#endif

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#   include <machine/endian.h>
#   if BYTE_ORDER == LITTLE_ENDIAN
#       define HOST_BYTEORDER_LE
#   else
#       define HOST_BYTEORDER_BE
#   endif
#endif

#if defined(__SunOS)
#   include <sys/byteorder.h>
#   if defined(_LITTLE_ENDIAN)
#       define HOST_BYTEORDER_LE
#   else
#       define HOST_BYTEORDER_BE
#   endif
#endif

#if defined(HOST_BYTEORDER_LE) && defined(HOST_BYTEORDER_BE)
#   error "Host endianess can't be both big and little!"
#elif defined(HOST_BYTEORDER_LE)

template <typename T>
inline T
letoh (T const& x)
{
    return x;
}

template <typename T>
inline T
betoh (T const& x)
{
    T copy(x);
    byte_swap<sizeof(T)>(reinterpret_cast<char*>(&copy));
    return copy;
}

#elif defined(HOST_BYTEORDER_BE)

template <typename T>
inline T
letoh (T const& x)
{
    T copy(x);
    byte_swap<sizeof(T)>(reinterpret_cast<char*>(&copy));
    return copy;
}

template <typename T>
inline T
betoh (T const& x)
{
    return x;
}

#else
#   error "Couldn't determine host endianess!"
#endif

UTIL_SYSTEM_NAMESPACE_END
UTIL_NAMESPACE_END

#endif /* UTIL_ENDIAN_HEADER */
