#include <iostream> // cout
#include <cstdlib> // ::exit
#include <csignal>
#include <execinfo.h> // ::backtrace

#include "util/system.h"

UTIL_NAMESPACE_BEGIN
UTIL_SYSTEM_NAMESPACE_BEGIN

void
signal_segfault_handler (int code)
{
    if (code != SIGSEGV)
        return;
    std::cerr << "Received signal SIGSEGV (segmentation fault)" << std::endl;
    print_stack_trace();
}

/* ---------------------------------------------------------------- */

void
print_stack_trace (void)
{
    /* Get stack pointers for all frames on the stack. */
    void *array[32];
    std::size_t size = ::backtrace(array, 32);

    /* Print out all the addresses to stderr. */
    std::cerr << "Obtained " << size << " stack frames:";
    for (std::size_t i = 0; i < size; ++i)
        std::cerr << " " << array[i];
    std::cerr << std::endl;

    /* Print out human readable representation to stderr. */
    ::backtrace_symbols_fd(array, size, 2); // 2 = stderr
    std::cerr << "Segmentation fault" << std::endl;
    ::exit(1);
}

UTIL_SYSTEM_NAMESPACE_END
UTIL_NAMESPACE_END
