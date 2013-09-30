/*
 * OpenGL shader program abstraction.
 * Written by Simon Fuhrmann.
 */

#ifndef OGL_SHADER_PROGRAM_HEADER
#define OGL_SHADER_PROGRAM_HEADER

#include <string>

#include "util/ref_ptr.h"
#include "math/vector.h"
#include "math/matrix.h"
#include "ogl/defines.h"
#include "ogl/opengl.h"

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
    typedef util::RefPtr<ShaderProgram> Ptr;
    typedef util::RefPtr<ShaderProgram const> ConstPtr;

private:
    GLuint prog_id;

    GLuint vert_id;
    GLuint geom_id;
    GLuint frag_id;

    std::string vert_filename;
    std::string geom_filename;
    std::string frag_filename;

private:
    ShaderProgram (void);

    void load_shader (GLuint& shader_id, GLuint shader_type,
        std::string const& filename);
    void unload_shader (GLuint& shader_id);
    void compile_shader (GLuint shader_id, std::string const& filename);

    GLint get_program_property (int pname);
    GLint get_shader_property (GLuint shader_id, int pname);

    void ensure_linked (void);

    void print_shader_log (GLuint shader_id);
    void link (void) const;

public:
    ~ShaderProgram (void);
    static Ptr create (void);

    /**
     * Loads all shaders by appending ".vert", ".geom"
     * and ".frag" to basename.
     */
    void load_all (std::string const& basename);

    /** Reloads all shaders from file. */
    void reload_all (void);

    /** Loads a vertex shader from file. */
    void load_vert (std::string const& filename);
    /** Loads optional geometry shader from file. */
    void load_geom (std::string const& filename);
    /** Load fragment shader from file. */
    void load_frag (std::string const& filename);

    /** Unloads vertex shader and resets stored filename. */
    void unload_vert (void);
    /** Unloads geometry shader and resets stored filename. */
    void unload_geom (void);
    /** Unloads fragment shader and resets stored filename. */
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
};

/* ---------------------------------------------------------------- */

inline
ShaderProgram::ShaderProgram (void)
{
    this->vert_id = 0;
    this->geom_id = 0;
    this->frag_id = 0;
    this->prog_id = glCreateProgram();
}

inline
ShaderProgram::~ShaderProgram (void)
{
    glDeleteProgram(this->prog_id);
    glDeleteShader(this->vert_id);
    glDeleteShader(this->geom_id);
    glDeleteShader(this->frag_id);
}

inline ShaderProgram::Ptr
ShaderProgram::create (void)
{
    return Ptr(new ShaderProgram);
}

inline void
ShaderProgram::load_vert (std::string const& filename)
{
    this->load_shader(this->vert_id, GL_VERTEX_SHADER, filename);
    this->vert_filename = filename;
}

inline void
ShaderProgram::load_geom (std::string const& filename)
{
    this->load_shader(this->geom_id, GL_GEOMETRY_SHADER, filename);
    this->geom_filename = filename;
}

inline void
ShaderProgram::load_frag (std::string const& filename)
{
    this->load_shader(this->frag_id, GL_FRAGMENT_SHADER, filename);
    this->frag_filename = filename;
}

inline void
ShaderProgram::unload_vert (void)
{
    this->vert_filename.clear();
    glDetachShader(this->prog_id, this->vert_id);
    glDeleteShader(this->vert_id);
}

inline void
ShaderProgram::unload_geom (void)
{
    this->geom_filename.clear();
    glDetachShader(this->prog_id, this->geom_id);
    glDeleteShader(this->geom_id);
}

inline void
ShaderProgram::unload_frag (void)
{
    this->frag_filename.clear();
    glDetachShader(this->prog_id, this->frag_id);
    glDeleteShader(this->frag_id);
}

inline GLint
ShaderProgram::get_attrib_location (char const* name)
{
    this->ensure_linked();
    return glGetAttribLocation(this->prog_id, name);
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
}

inline void
ShaderProgram::unbind (void) const
{
    glUseProgram(0);
}

inline void
ShaderProgram::link (void) const
{
    glLinkProgram(this->prog_id);
}

inline void
ShaderProgram::ensure_linked (void)
{
    if (this->get_program_property(GL_LINK_STATUS) == GL_FALSE)
        this->link();
}

inline GLint
ShaderProgram::get_program_property (int pname)
{
    GLint ret;
    glGetProgramiv(this->prog_id, pname, &ret);
    return ret;
}

inline GLint
ShaderProgram::get_shader_property (GLuint shader_id, int pname)
{
    GLint ret;
    glGetShaderiv(shader_id, pname, &ret);
    return ret;
}

OGL_NAMESPACE_END

#endif /* OGL_SHADER_PROGRAM_HEADER */
