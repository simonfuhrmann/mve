/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef OGL_VERTEX_ARRAY_HEADER
#define OGL_VERTEX_ARRAY_HEADER

#include <utility>
#include <vector>
#include <string>
#include <memory>

#include "ogl/defines.h"
#include "ogl/opengl.h"
#include "ogl/check_gl_error.h"
#include "ogl/shader_program.h"
#include "ogl/vertex_buffer.h"

OGL_NAMESPACE_BEGIN

/**
 * OpenGL vertex array object abstraction.
 *
 * Vertex buffer objects (VBOs) may be plugged into this class
 * to compose more complex objects such as colored point sets or
 * meshes with generic per-vertex attributes. Per-vertex attributes
 * are named and automatically associated with shader input variables.
 */
class VertexArray
{
public:
    typedef std::shared_ptr<VertexArray> Ptr;
    typedef std::shared_ptr<VertexArray const> ConstPtr;

    typedef std::pair<VertexBuffer::Ptr, std::string> BoundVBO;
    typedef std::vector<BoundVBO> VBOList;

public:
    virtual ~VertexArray (void);
    static Ptr create (void);

    /** Sets the primitive type to be used with the corresponding draw call. */
    void set_primitive (GLuint primitive);

    /** Assigns a shader that is used for drawing the vertex array. */
    void set_shader (ShaderProgram::Ptr shader);

    /** Sets the vertex VBO with vertex positions. */
    void set_vertex_vbo (VertexBuffer::Ptr vbo);

    /** Sets the vertex indices VBO with triangle definitions. */
    void set_index_vbo (VertexBuffer::Ptr vbo);

    /** Adds a generic VBO with attribute name. */
    void add_vbo (VertexBuffer::Ptr vbo, std::string const& name);

    /** Removes a VBO from the list. */
    void remove_vbo (std::string const& name);

    /** Removes VBOs and creates a new vertex array. */
    void reset_vertex_array (void);

    /** Binds the shader and issues drawing commands. */
    void draw (void);

protected:
    VertexArray (void);
    void assign_attrib (BoundVBO const& bound_vbo);

private:
    GLuint vao_id;
    GLuint primitive;
    ShaderProgram::Ptr shader;

    /* Vertex VBO, Index VBO and generic VBOs. */
    VertexBuffer::Ptr vert_vbo;
    VertexBuffer::Ptr index_vbo;
    VBOList vbo_list;
};

/* ---------------------------------------------------------------- */

inline
VertexArray::VertexArray (void)
{
    glGenVertexArrays(1, &this->vao_id);
    check_gl_error();
    this->primitive = GL_TRIANGLES;
}

inline
VertexArray::~VertexArray (void)
{
    glDeleteVertexArrays(1, &this->vao_id);
    check_gl_error();
}

inline VertexArray::Ptr
VertexArray::create (void)
{
    return Ptr(new VertexArray);
}

inline void
VertexArray::set_primitive (GLuint primitive)
{
    this->primitive = primitive;
}

inline void
VertexArray::set_vertex_vbo (VertexBuffer::Ptr vbo)
{
    this->vert_vbo = vbo;
}

inline void
VertexArray::set_index_vbo (VertexBuffer::Ptr vbo)
{
    this->index_vbo = vbo;
}

inline void
VertexArray::add_vbo (VertexBuffer::Ptr vbo, std::string const& name)
{
    this->vbo_list.push_back(std::make_pair(vbo, name));
}

inline void
VertexArray::reset_vertex_array(void)
{
    this->vert_vbo.reset();
    this->index_vbo.reset();
    this->vbo_list.clear();
    //this->shader.reset();
    glDeleteVertexArrays(1, &this->vao_id);
    check_gl_error();
    glGenVertexArrays(1, &this->vao_id);
    check_gl_error();
}

inline void
VertexArray::set_shader (ShaderProgram::Ptr shader)
{
    this->shader = shader;
}

OGL_NAMESPACE_END

#endif /* OGL_VERTEX_ARRAY_HEADER */
