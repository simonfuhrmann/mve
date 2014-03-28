#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>

#include <cstring>
#include <cerrno>

#include "util/exception.h"
#include "util/file_system.h"
#include "ogl/shader_program.h"

OGL_NAMESPACE_BEGIN

void
ShaderProgram::load_shader_file (GLuint& shader_id, GLuint shader_type,
    std::string const& filename)
{
    std::string shader_code;
    util::fs::read_file_to_string(filename, &shader_code);

    try
    {
        this->load_shader_code(shader_id, shader_type, shader_code);
    }
    catch (util::Exception& e)
    {
        throw util::Exception(filename + ": " + e);
    }
}

void
ShaderProgram::load_shader_code (GLuint& shader_id, GLuint shader_type,
    std::string const& code)
{
    if (shader_id == 0)
    {
        shader_id = glCreateShader(shader_type);
        check_gl_error();

        glAttachShader(this->prog_id, shader_id);
        check_gl_error();
    }

    this->compile_shader(shader_id, code);
}

/* ---------------------------------------------------------------- */

void
ShaderProgram::unload_shader (GLuint& shader_id)
{
    if (shader_id != 0)
    {
        glDetachShader(this->prog_id, shader_id);
        check_gl_error();
        glDeleteShader(shader_id);
        check_gl_error();
        shader_id = 0;
    }
}

/* ---------------------------------------------------------------- */

void
ShaderProgram::compile_shader (GLuint shader_id, std::string const& code)
{
    /* Pass code to OpenGL. */
    char const* data[1] = { code.c_str() };
    glShaderSource(shader_id, 1, data, NULL);
    check_gl_error();

    /* Compile shader. */
    glCompileShader(shader_id);
    check_gl_error();
    if (this->get_shader_property(shader_id, GL_COMPILE_STATUS) == GL_FALSE)
    {
        GLint log_size = this->get_shader_property(shader_id, GL_INFO_LOG_LENGTH);
        if (log_size == 0)
               throw util::Exception("Shader compilation failed (no message).");

        std::string log;
        log.append(log_size + 1, '\0');
        glGetShaderInfoLog(shader_id, log_size + 1, NULL, &log[0]);
        throw util::Exception(log);
    }
}

/* ---------------------------------------------------------------- */

void
ShaderProgram::try_load_all (std::string const& basename, bool unload)
{
    std::string vert_filename = basename + ".vert";
    if (util::fs::file_exists(vert_filename.c_str()))
        this->load_vert_file(vert_filename);
    else if (unload)
        this->unload_vert();

    std::string geom_filename = basename + ".geom";
    if (util::fs::file_exists(geom_filename.c_str()))
        this->load_geom_file(geom_filename);
    else if (unload)
        this->unload_geom();

    std::string frag_filename = basename + ".frag";
    if (util::fs::file_exists(frag_filename.c_str()))
        this->load_frag_file(frag_filename);
    else if (unload)
        this->unload_frag();
}

OGL_NAMESPACE_END
