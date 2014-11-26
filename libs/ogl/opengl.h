/*
 * This file solely exists to include the OpenGL API header file(s).
 *
 * Since this may vary depending on the specific application, this
 * file may be modified according to user demands. The default
 * implementation expects that GLEW is installed system-wide and
 * is initialized before any OpenGL API function is called.
 *
 * Written by Simon Fuhrmann.
 */

#ifndef OGL_OPEN_GL_HEADER
#define OGL_OPEN_GL_HEADER

#if defined(OGL_USE_OSMESA)
#  define GL_GLEXT_PROTOTYPES
#  include <GL/osmesa.h>
#elif defined(__APPLE__)
#  include <OpenGL/gl3.h>
#elif defined(_WIN32)
#  include <GL/glew.h>
#else
#  define GL_GLEXT_PROTOTYPES
#  include <GL/gl.h>
#  include <GL/glext.h>
#endif

#endif /* OGL_OPEN_GL_HEADER */
