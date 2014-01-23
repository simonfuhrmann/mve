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
ShaderProgram::load_shader (GLuint& shader_id, GLuint shader_type,
    std::string const& filename)
{
    if (filename.empty())
        throw std::invalid_argument("Shader: No filename given");

    if (shader_id == 0)
    {
        shader_id = glCreateShader(shader_type);
        glAttachShader(this->prog_id, shader_id);
    }

    this->compile_shader(shader_id, filename);
}

/* ---------------------------------------------------------------- */

void
ShaderProgram::reload_all (void)
{
    this->load_shader(this->vert_id, GL_VERTEX_SHADER, this->vert_filename);
    if (!this->geom_filename.empty())
    {
        this->load_shader(this->geom_id, GL_GEOMETRY_SHADER,
            this->geom_filename);
    }
    this->load_shader(this->frag_id, GL_FRAGMENT_SHADER, this->frag_filename);
    this->link();
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
ShaderProgram::compile_shader (GLuint shader_id, std::string const& filename)
{
    /* Load source code. */
    std::ifstream in(filename.c_str());
    if (!in.good())
        throw util::FileException(filename, ::strerror(errno));

    std::string code(std::istreambuf_iterator<char>(in),
        (std::istreambuf_iterator<char>()));

    in.close();

    /* Pass code to OpenGL. */
    {
        char const* data[1];
        data[0] = code.c_str();
        glShaderSource(shader_id, 1, data, NULL);
        code.clear();
    }

    /* Compile shader. */
    glCompileShader(shader_id);
    if (this->get_shader_property(shader_id, GL_COMPILE_STATUS) == GL_FALSE)
    {
        std::cout << std::endl;
        std::cout << "ERROR compiling shader " << filename << std::endl;
        this->print_shader_log(shader_id);
    }
}

/* ---------------------------------------------------------------- */

void
ShaderProgram::load_all (std::string const& basename)
{
    this->vert_filename = basename + ".vert";
    this->load_vert(this->vert_filename);

    this->geom_filename = basename + ".geom";
    if (util::fs::file_exists(this->geom_filename.c_str()))
        this->load_geom(this->geom_filename);
    else
        this->unload_geom();

    this->frag_filename = basename + ".frag";
    this->load_frag(this->frag_filename);
}

/* ---------------------------------------------------------------- */

GLint
ShaderProgram::get_uniform_location (char const* name)
{
    this->ensure_linked();
    GLint loc = glGetUniformLocation(this->prog_id, name);
    //std::cout << "Uniform location for " << name << ": " << loc << std::endl;
    return loc;
}

/* ---------------------------------------------------------------- */

void
ShaderProgram::print_shader_log (GLuint shader_id)
{
    GLint log_size = this->get_shader_property(shader_id, GL_INFO_LOG_LENGTH);
    if (log_size == 0)
    {
        std::cout << "Shader compile log is empty!" << std::endl;
        return;
    }

    std::vector<char> log;
    log.resize(log_size + 1, '\0');
    glGetShaderInfoLog(shader_id, log_size + 1, NULL, &log[0]);
    std::cout.write(&log[0], log_size);
}

OGL_NAMESPACE_END
