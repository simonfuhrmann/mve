/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef OGL_MESH_RENDERER_HEADER
#define OGL_MESH_RENDERER_HEADER

#include <memory>

#include "mve/mesh.h"
#include "ogl/defines.h"
#include "ogl/opengl.h"
#include "ogl/shader_program.h"
#include "ogl/vertex_array.h"
#include "ogl/vertex_buffer.h"

OGL_NAMESPACE_BEGIN

/**
 * OpenGL renderer that takes a mesh and automatically creates
 * the appropriate VBOs and a vertex array object.
 */
class MeshRenderer : public VertexArray
{
public:
    typedef std::shared_ptr<MeshRenderer> Ptr;
    typedef std::shared_ptr<MeshRenderer const> ConstPtr;

public:
    static Ptr create (void);
    static Ptr create (mve::TriangleMesh::ConstPtr mesh);
    void set_mesh (mve::TriangleMesh::ConstPtr mesh);

private:
    MeshRenderer (void);
    MeshRenderer (mve::TriangleMesh::ConstPtr mesh);
};

/* ---------------------------------------------------------------- */

inline MeshRenderer::Ptr
MeshRenderer::create (void)
{
    return Ptr(new MeshRenderer());
}

inline MeshRenderer::Ptr
MeshRenderer::create (mve::TriangleMesh::ConstPtr mesh)
{
    return Ptr(new MeshRenderer(mesh));
}

inline
MeshRenderer::MeshRenderer (void)
{
}

inline
MeshRenderer::MeshRenderer (mve::TriangleMesh::ConstPtr mesh)
{
    this->set_mesh(mesh);
}

OGL_NAMESPACE_END

#endif /* OGL_MESH_RENDERER_HEADER */
