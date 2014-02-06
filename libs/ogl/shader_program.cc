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
    /* Load source code. */
    std::ifstream in(filename.c_str());
    if (!in.good())
        throw util::FileException(filename, ::strerror(errno));

    /*
     * Note that the parentheses around the second parameter are required
     * to disambiguate between a call to the std::string constructor with
     * two instances of std::istreambuf_iterator and the declaration of a
     * function with the following signature:
     * std::string code(std::istreambuf_iterator<char, std::char_traits<char> >, std::istreambuf_iterator<char, std::char_traits<char> > (*)())
     */
    std::string code(std::istreambuf_iterator<char>(in),
        (std::istreambuf_iterator<char>()));

    in.close();

    try
    {
        this->load_shader_code(shader_id, shader_type, code);
    }
    catch (util::Exception &e)
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
        glAttachShader(this->prog_id, shader_id);
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
        glDeleteShader(shader_id);
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

    /* Compile shader. */
    glCompileShader(shader_id);
    if (this->get_shader_property(shader_id, GL_COMPILE_STATUS) == GL_FALSE)
    {
        GLint log_size = this->get_shader_property(shader_id, GL_INFO_LOG_LENGTH);
        if (log_size == 0)
        	   throw util::Exception("Shader compilation failed (no message).");

        std::vector<char> log(log_size + 1, '\0');
        glGetShaderInfoLog(shader_id, log_size + 1, NULL, &log[0]);
		  throw util::Exception(std::string(&log[0], log_size));
    }
}

/* ---------------------------------------------------------------- */

void
ShaderProgram::load_all (std::string const& basename)
{
    this->load_vert_file(basename + ".vert");

    std::string geom_filename = basename + ".geom";
    if (util::fs::file_exists(geom_filename.c_str()))
        this->load_geom_file(geom_filename);
    else
        this->unload_geom();

    this->load_frag_file(basename + ".frag");
}

OGL_NAMESPACE_END
