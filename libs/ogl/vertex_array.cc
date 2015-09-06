/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <algorithm>
#include <stdexcept>

#include "ogl/vertex_array.h"

OGL_NAMESPACE_BEGIN

struct VBORemovePred
{
    std::string name;
    VBORemovePred (std::string const& name) : name(name) {}
    bool operator() (VertexArray::BoundVBO const& o)
    { return o.second == name; }
};

void
VertexArray::remove_vbo (std::string const& name)
{
    VBOList::iterator new_end = std::remove_if(this->vbo_list.begin(),
        this->vbo_list.end(), VBORemovePred(name));
    this->vbo_list.erase(new_end, this->vbo_list.end());
}

/* ---------------------------------------------------------------- */

void
VertexArray::assign_attrib (BoundVBO const& bound_vbo)
{
    VertexBuffer::Ptr vbo = bound_vbo.first;
    std::string const& name = bound_vbo.second;

    GLint location = this->shader->get_attrib_location(name.c_str());
    if (location < 0)
        return;

    vbo->bind();
    glVertexAttribPointer(location, vbo->get_values_per_vertex(),
        vbo->get_data_type(), GL_TRUE, 0, nullptr);
    check_gl_error();
    glEnableVertexAttribArray(location);
    check_gl_error();
}

/* ---------------------------------------------------------------- */

void
VertexArray::draw (void)
{
    if (this->vert_vbo == nullptr)
        throw std::runtime_error("No vertex VBO set!");

    if (this->shader == nullptr)
        throw std::runtime_error("No shader program set!");

    /* Make current vertex array active. */
    glBindVertexArray(this->vao_id);
    check_gl_error();

    /* Bind the shader program. */
    this->shader->bind();

    /* Assign vertex positions attribute. */
    this->assign_attrib(BoundVBO(this->vert_vbo, OGL_ATTRIB_POSITION));

    /* Assign generic vertex attributes. */
    for (std::size_t i = 0; i < this->vbo_list.size(); ++i)
        this->assign_attrib(this->vbo_list[i]);

    /* Draw triangles if indices are given, draw points otherwise. */
    if (this->index_vbo != nullptr)
    {
        this->index_vbo->bind();
        glDrawElements(this->primitive, this->index_vbo->get_element_amount(),
            GL_UNSIGNED_INT, nullptr);
        check_gl_error();
    }
    else
    {
        glDrawArrays(this->primitive, 0, this->vert_vbo->get_element_amount());
        check_gl_error();
    }

    this->shader->unbind();
    glBindVertexArray(0);
    check_gl_error();
}

OGL_NAMESPACE_END
