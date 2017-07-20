/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

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
    this->need_to_link = true;
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
    glShaderSource(shader_id, 1, data, nullptr);
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
        glGetShaderInfoLog(shader_id, log_size + 1, nullptr, &log[0]);
        throw util::Exception(log);
    }
}

void
ShaderProgram::ensure_linked (void)
{
    if (this->need_to_link)
    {
        glLinkProgram(this->prog_id);
        check_gl_error();
        if (this->get_program_property(GL_LINK_STATUS) == GL_FALSE)
        {
            GLint log_size = this->get_program_property(GL_INFO_LOG_LENGTH);
            if (log_size == 0)
                throw util::Exception("Failed to link program (no message).");

            std::string log;
            log.append(log_size + 1, '\0');
            glGetProgramInfoLog(this->prog_id, log_size + 1, nullptr, &log[0]);
            throw util::Exception(log);
        }
        this->need_to_link = false;
    }
}

/* ---------------------------------------------------------------- */

bool
ShaderProgram::try_load_all (std::string const& basename)
{
    std::string vert_filename = basename + ".vert";
    std::string geom_filename = basename + ".geom";
    std::string frag_filename = basename + ".frag";

    if (!util::fs::file_exists(vert_filename.c_str())
     || !util::fs::file_exists(frag_filename.c_str()))
    {
        std::cerr << "Skipping shaders from " << basename << ".*" << std::endl;
        return false;
    }

    std::cerr << "Loading shaders from " << basename << ".*" << std::endl;

    this->load_vert_file(vert_filename);

    if (util::fs::file_exists(geom_filename.c_str()))
        this->load_geom_file(geom_filename);

    this->load_frag_file(frag_filename);

    return true;
}

OGL_NAMESPACE_END
