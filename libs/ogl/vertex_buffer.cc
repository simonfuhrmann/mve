/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include "ogl/vertex_buffer.h"

OGL_NAMESPACE_BEGIN

VertexBuffer::VertexBuffer (void)
{
    glGenBuffers(1, &this->vbo_id);
    check_gl_error();
    this->vbo_target = GL_ARRAY_BUFFER;
    this->datatype = GL_FLOAT;
    this->usage = GL_STATIC_DRAW;
    this->bytes = 0;
    this->vpv = 0;
    this->elems = 0;
    this->stride = 0;
}

/* ---------------------------------------------------------------- */

void
VertexBuffer::set_data (GLfloat const* data, GLsizei elems, GLint vpv)
{
    this->vbo_target = GL_ARRAY_BUFFER;
    this->datatype = GL_FLOAT;
    this->bytes = elems * vpv * sizeof(GLfloat);
    this->vpv = vpv;
    this->elems = elems;

    this->bind();
    glBufferData(this->vbo_target, this->bytes, data, this->usage);
    check_gl_error();
}

/* ---------------------------------------------------------------- */

void
VertexBuffer::set_data (GLubyte const* data, GLsizei elems, GLint vpv)
{
    this->vbo_target = GL_ARRAY_BUFFER;
    this->datatype = GL_UNSIGNED_BYTE;
    this->bytes = elems * vpv * sizeof(GLubyte);
    this->vpv = vpv;
    this->elems = elems;

    this->bind();
    glBufferData(this->vbo_target, this->bytes, data, this->usage);
    check_gl_error();
}

/* ---------------------------------------------------------------- */

void
VertexBuffer::set_indices (GLuint const* data, GLsizei num_indices)
{
    this->vbo_target = GL_ELEMENT_ARRAY_BUFFER;
    this->datatype = GL_UNSIGNED_INT;
    this->bytes = num_indices * sizeof(unsigned int);
    this->vpv = 3;
    this->elems = num_indices;

    this->bind();
    glBufferData(this->vbo_target, this->bytes, data, this->usage);
    check_gl_error();
}

OGL_NAMESPACE_END
