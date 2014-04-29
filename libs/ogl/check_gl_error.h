#ifndef OGL_CHECK_GL_ERROR_HEADER
#define OGL_CHECK_GL_ERROR_HEADER

#include <stdexcept>

#include "ogl/defines.h"
#include "ogl/opengl.h"
#include "util/string.h"

OGL_NAMESPACE_BEGIN

inline void
check_gl_error()
{
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
        throw std::runtime_error("GL error: " + util::string::get(err));
}

OGL_NAMESPACE_END

#endif
