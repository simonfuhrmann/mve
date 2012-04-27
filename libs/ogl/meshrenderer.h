/*
 * OpenGL mesh renderer using VBOs.
 * Written by Simon Fuhrmann.
 * TODO: Make VBO abstraction, add those generically
 */
#ifndef OGL_MESH_RENDERER_HEADER
#define OGL_MESH_RENDERER_HEADER

#include "util/refptr.h"

#include "mve/trianglemesh.h"

#include "defines.h"
#include "opengl.h"
#include "shaderprogram.h"
#include "vertexarray.h"
#include "vertexbuffer.h"

OGL_NAMESPACE_BEGIN

/**
 * OpenGL renderer that takes a mesh and automatically creates
 * the appropriate VBOs and a vertex array object.
 */
class MeshRenderer : public VertexArray
{
public:
    typedef util::RefPtr<MeshRenderer> Ptr;
    typedef util::RefPtr<MeshRenderer const> ConstPtr;

private:
    MeshRenderer (void);
    MeshRenderer (mve::TriangleMesh::ConstPtr mesh);

public:
    static Ptr create (void);
    static Ptr create (mve::TriangleMesh::ConstPtr mesh);

    void set_mesh (mve::TriangleMesh::ConstPtr mesh);
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
