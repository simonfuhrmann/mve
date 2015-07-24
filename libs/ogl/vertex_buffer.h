/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef OGL_VERTEX_BUFFER_HEADER
#define OGL_VERTEX_BUFFER_HEADER

#include <memory>

#include "ogl/defines.h"
#include "ogl/opengl.h"
#include "ogl/check_gl_error.h"

OGL_NAMESPACE_BEGIN

/**
 * OpenGL vertex buffer object (VBO) abstraction.
 *
 * A vertex buffer object stores large chunks of data, for example
 * per-vertex attributes such as positions, normals or colors or
 * primitive connectivity such as triangle index lists. Instances
 * of this class may be plugged in vertex arrays to efficiently
 * render more complex objects such as point sets or meshes.
 */
class VertexBuffer
{
public:
    typedef std::shared_ptr<VertexBuffer> Ptr;
    typedef std::shared_ptr<VertexBuffer const> ConstPtr;

public:
    ~VertexBuffer (void);
    static Ptr create (void);

    /**
     * Sets the VBO usage flag. GL_STATIC_DRAW is default.
     * Call this before setting data to create proper vertex buffer.
     */
    void set_usage (GLenum usage);

    /**
     * Sets the data stride, i.e. bytes between subsequent values.
     * Call this before setting data to create proper vertex buffer.
     */
    void set_stride (GLsizei stride);

    /** Sets data for the VBO: amount of elements and values per vertex. */
    void set_data (GLfloat const* data, GLsizei elems, GLint vpv);
    /** Sets data for the VBO: amount of elements and values per vertex. */
    void set_data (GLubyte const* data, GLsizei elems, GLint vpv);
    /** Sets index data for the VBO. Triangles are assumed, i.e. vpv = 3. */
    void set_indices (GLuint const* data, GLsizei num_indices);

    /** Returns the VBO target, e.g. ARRAY_BUFFER or ELEMENT_ARRAY_BUFFER. */
    GLenum get_vbo_target (void) const;
    /** Returns the data type of the VBO data. */
    GLenum get_data_type (void) const;
    /** Returns the VBO usage flag. */
    GLenum get_vbo_usage (void) const;
    /** Returns the amount of bytes of the VBO. */
    GLsizeiptr get_byte_size (void) const;
    /** Returns the amount of values per vertex. */
    GLint get_values_per_vertex (void) const;
    /** Returns the amount of elements (attributes or primitives). */
    GLsizei get_element_amount (void) const;
    /** Returns the data stride. */
    GLsizei get_stride (void) const;

    /** Binds the VBO. */
    void bind (void);

private:
    VertexBuffer (void);

private:
    GLuint vbo_id;
    GLenum vbo_target;
    GLenum datatype;
    GLenum usage;
    GLsizeiptr bytes;
    GLint vpv;
    GLsizei elems;
    GLsizei stride;
};

/* ---------------------------------------------------------------- */

inline
VertexBuffer::~VertexBuffer (void)
{
    glDeleteBuffers(1, &this->vbo_id);
    check_gl_error();
}

inline VertexBuffer::Ptr
VertexBuffer::create (void)
{
    return Ptr(new VertexBuffer);
}

inline void
VertexBuffer::set_stride (GLsizei stride)
{
    this->stride = stride;
}

inline void
VertexBuffer::set_usage (GLenum usage)
{
    this->usage = usage;
}

inline GLenum
VertexBuffer::get_vbo_target (void) const
{
    return this->vbo_target;
}

inline GLsizei
VertexBuffer::get_stride (void) const
{
    return this->stride;
}

inline GLint
VertexBuffer::get_values_per_vertex (void) const
{
    return this->vpv;
}

inline GLsizeiptr
VertexBuffer::get_byte_size (void) const
{
    return this->bytes;
}

inline GLsizei
VertexBuffer::get_element_amount (void) const
{
    return this->elems;
}

inline GLenum
VertexBuffer::get_vbo_usage (void) const
{
    return this->usage;
}

inline GLenum
VertexBuffer::get_data_type (void) const
{
    return this->datatype;
}

inline void
VertexBuffer::bind (void)
{
    glBindBuffer(this->vbo_target, this->vbo_id);
}

OGL_NAMESPACE_END

#endif /* OGL_VERTEX_BUFFER_HEADER */
