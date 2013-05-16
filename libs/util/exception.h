/*
 * Universal exception classes go here.
 * Written by Simon Fuhrmann.
 */

#ifndef UTIL_EXCEPTION_HEADER
#define UTIL_EXCEPTION_HEADER

#include <string>
#include <stdexcept>

#include "util/defines.h"

UTIL_NAMESPACE_BEGIN

/**
 * Universal, simple exception class.
 */
class Exception : public std::exception, public std::string
{
  public:
    Exception (void) throw()
    { }

    Exception (std::string const& msg) throw() : std::string(msg)
    { }

    Exception (std::string const& msg, char const* msg2) throw()
        : std::string(msg)
    { this->append(msg2); }

    Exception (std::string const& msg, std::string const& msg2) throw()
        : std::string(msg)
    { this->append(msg2); }

    virtual ~Exception (void) throw()
    { }

    virtual const char* what (void) const throw()
    { return this->c_str(); }
};

/* ---------------------------------------------------------------- */

/**
 * Exception class for file exceptions with additional filename.
 */
class FileException : public Exception
{
public:
    std::string filename;

public:
    FileException(std::string const& filename, std::string const& msg) throw()
        : Exception(msg), filename(filename)
    { }

    FileException(std::string const& filename, char const* msg) throw()
        : Exception(msg), filename(filename)
    { }

    virtual ~FileException (void) throw()
    { }
};

UTIL_NAMESPACE_END

#endif /* UTIL_EXCEPTION_HEADER */
