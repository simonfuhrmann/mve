/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef OGL_SHADER_PROGRAM_HEADER
#define OGL_SHADER_PROGRAM_HEADER

#include <string>
#include <memory>

#include "math/vector.h"
#include "math/matrix.h"
#include "ogl/defines.h"
#include "ogl/opengl.h"
#include "ogl/check_gl_error.h"

#define OGL_ATTRIB_POSITION "pos"
#define OGL_ATTRIB_NORMAL "normal"
#define OGL_ATTRIB_COLOR "color"
#define OGL_ATTRIB_TEXCOORD "texuv"

OGL_NAMESPACE_BEGIN

/**
 * Abstraction for OpenGL Shader Programs.
 */
class ShaderProgram
{
public:
    typedef std::shared_ptr<ShaderProgram> Ptr;
    typedef std::shared_ptr<ShaderProgram const> ConstPtr;

public:
    ~ShaderProgram (void);
    static Ptr create (void);

    /**
     * Try loading all shaders by appending ".vert", ".geom"
     * and ".frag" to basename.
     */
    bool try_load_all (std::string const& basename);

    /** Loads a vertex shader from file. */
    void load_vert_file (std::string const& filename);
    /** Loads optional geometry shader from file. */
    void load_geom_file (std::string const& filename);
    /** Load fragment shader from file. */
    void load_frag_file (std::string const& filename);

    /** Loads a vertex shader from code in memory. */
    void load_vert_code (std::string const& code);
    /** Loads optional geometry shader from code in memory. */
    void load_geom_code (std::string const& code);
    /** Load fragment shader from code in memory. */
    void load_frag_code (std::string const& code);

    /** Unloads vertex shader. */
    void unload_vert (void);
    /** Unloads geometry shader. */
    void unload_geom (void);
    /** Unloads fragment shader. */
    void unload_frag (void);

    /**
     * Returns attribute location for the program. If the program
     * has not yet been linked, it is linked first. If there is no
     * attribute by that name, -1 is returned.
     */
    GLint get_attrib_location (char const* name);

    /**
     * Returns the uniform location of the program. If the program
     * has not yet been linked, it is linked first. If there is no
     * uniform variable by that name, -1 is returned.
     */
    GLint get_uniform_location (char const* name);

    /** Sends 3-vector 'v' to uniform location 'name'. */
    void send_uniform (char const* name, math::Vec3f const& v);
    /** Sends 4-vector 'v' to uniform location 'name'. */
    void send_uniform (char const* name, math::Vec4f const& v);
    /** Sends 4x4-matrx 'm' to uniform location 'name'. */
    void send_uniform (char const* name, math::Matrix4f const& m);
    /** Sends integer 'val' to uniform location 'name'. */
    void send_uniform (char const* name, GLint val);
    /** Sends float 'val' to uniform location 'name'. */
    void send_uniform (char const* name, GLfloat val);

    /** Selects the shader program for rendering. */
    void bind (void);

    /** Deselects the currend shader program. */
    void unbind (void) const;

private:
    ShaderProgram (void);

    void load_shader_file (GLuint& shader_id, GLuint shader_type,
        std::string const& filename);
    void load_shader_code (GLuint& shader_id, GLuint shader_type,
        std::string const& code);
    void unload_shader (GLuint& shader_id);
    void compile_shader (GLuint shader_id, std::string const& code);

    GLint get_program_property (int pname);
    GLint get_shader_property (GLuint shader_id, int pname);

    void ensure_linked (void);

private:
    GLuint prog_id;
    GLuint vert_id;
    GLuint geom_id;
    GLuint frag_id;

    bool need_to_link;
};

/* ---------------------------------------------------------------- */

inline
ShaderProgram::ShaderProgram (void)
{
    this->vert_id = 0;
    this->geom_id = 0;
    this->frag_id = 0;
    this->prog_id = glCreateProgram();
    check_gl_error();
    this->need_to_link = false;
}

inline
ShaderProgram::~ShaderProgram (void)
{
    glDeleteProgram(this->prog_id);
    check_gl_error();
    glDeleteShader(this->vert_id);
    check_gl_error();
    glDeleteShader(this->geom_id);
    check_gl_error();
    glDeleteShader(this->frag_id);
    check_gl_error();
}

inline ShaderProgram::Ptr
ShaderProgram::create (void)
{
    return Ptr(new ShaderProgram);
}

inline void
ShaderProgram::load_vert_file (std::string const& filename)
{
    this->load_shader_file(this->vert_id, GL_VERTEX_SHADER, filename);
}

inline void
ShaderProgram::load_geom_file (std::string const& filename)
{
    this->load_shader_file(this->geom_id, GL_GEOMETRY_SHADER, filename);
}

inline void
ShaderProgram::load_frag_file (std::string const& filename)
{
    this->load_shader_file(this->frag_id, GL_FRAGMENT_SHADER, filename);
}

inline void
ShaderProgram::load_vert_code (std::string const& code)
{
    this->load_shader_code(this->vert_id, GL_VERTEX_SHADER, code);
}

inline void
ShaderProgram::load_geom_code (std::string const& code)
{
    this->load_shader_code(this->geom_id, GL_GEOMETRY_SHADER, code);
}

inline void
ShaderProgram::load_frag_code (std::string const& code)
{
    this->load_shader_code(this->frag_id, GL_FRAGMENT_SHADER, code);
}

inline void
ShaderProgram::unload_vert (void)
{
    glDetachShader(this->prog_id, this->vert_id);
    check_gl_error();

    glDeleteShader(this->vert_id);
    check_gl_error();

    this->vert_id = 0;
}

inline void
ShaderProgram::unload_geom (void)
{
    glDetachShader(this->prog_id, this->geom_id);
    check_gl_error();

    glDeleteShader(this->geom_id);
    check_gl_error();

    this->geom_id = 0;
}

inline void
ShaderProgram::unload_frag (void)
{
    glDetachShader(this->prog_id, this->frag_id);
    check_gl_error();

    glDeleteShader(this->frag_id);
    check_gl_error();

    this->frag_id = 0;
}

inline GLint
ShaderProgram::get_attrib_location (char const* name)
{
    this->ensure_linked();
    return glGetAttribLocation(this->prog_id, name);
}

inline GLint
ShaderProgram::get_uniform_location (char const* name)
{
    this->ensure_linked();
    GLint loc = glGetUniformLocation(this->prog_id, name);
    return loc;
}

inline void
ShaderProgram::send_uniform (char const* name, math::Vec3f const& v)
{
    GLint loc = this->get_uniform_location(name);
    if (loc < 0)
        return;
    glUniform3fv(loc, 1, *v);
}

inline void
ShaderProgram::send_uniform (char const* name, math::Vec4f const& v)
{
    GLint loc = this->get_uniform_location(name);
    if (loc < 0)
        return;
    glUniform4fv(loc, 1, *v);
}

inline void
ShaderProgram::send_uniform (const char* name, math::Matrix4f const& m)
{
    GLint loc = this->get_uniform_location(name);
    if (loc < 0)
        return;
    glUniformMatrix4fv(loc, 1, true, *m);
}

inline void
ShaderProgram::send_uniform (const char* name, GLint val)
{
    GLint loc = this->get_uniform_location(name);
    if (loc < 0)
        return;
    glUniform1i(loc, val);
}

inline void
ShaderProgram::send_uniform (const char* name, GLfloat val)
{
    GLint loc = this->get_uniform_location(name);
    if (loc < 0)
        return;
    glUniform1f(loc, val);
}

inline void
ShaderProgram::bind (void)
{
    this->ensure_linked();
    glUseProgram(this->prog_id);
    check_gl_error();
}

inline void
ShaderProgram::unbind (void) const
{
    glUseProgram(0);
    check_gl_error();
}

inline void
ShaderProgram::ensure_linked (void)
{
    if (this->need_to_link)
    {
        glLinkProgram(this->prog_id);
        check_gl_error();
        this->need_to_link = false;
    }
}

inline GLint
ShaderProgram::get_program_property (int pname)
{
    GLint ret;
    glGetProgramiv(this->prog_id, pname, &ret);
    check_gl_error();
    return ret;
}

inline GLint
ShaderProgram::get_shader_property (GLuint shader_id, int pname)
{
    GLint ret;
    glGetShaderiv(shader_id, pname, &ret);
    check_gl_error();
    return ret;
}

OGL_NAMESPACE_END

#endif /* OGL_SHADER_PROGRAM_HEADER */
