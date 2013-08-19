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

#ifdef OGL_USE_OSMESA
#  define GL_GLEXT_PROTOTYPES
#  include <GL/osmesa.h>
#else
#  include <GL/glew.h>
#endif

#endif /* OGL_OPEN_GL_HEADER */
