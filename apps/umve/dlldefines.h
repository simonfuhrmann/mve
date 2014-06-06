#ifndef UMVE_DLLDEFINES_HEADER
#define UMVE_DLLDEFINES_HEADER

#if defined (_WIN32) 
#   if defined(COMPILING_PLUGIN)
#       define UMVE_API __declspec(dllimport)
#   else
#       define UMVE_API __declspec(dllexport)
#   endif
#elif __GNUC__ > 3
#   define UMVE_API __attribute__((visibility("default"))
#else
#   define UMVE_API
#endif

#endif /* UMVE_DLLDEFINES_HEADER */

