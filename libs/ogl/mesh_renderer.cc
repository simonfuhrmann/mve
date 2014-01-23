#include "ogl/opengl.h"
#include "ogl/mesh_renderer.h"

OGL_NAMESPACE_BEGIN

void
MeshRenderer::set_mesh (mve::TriangleMesh::ConstPtr mesh)
{
    if (mesh == NULL)
        throw std::invalid_argument("Got NULL mesh");

    /* Clean previous content. */
    this->reset_vertex_array();

    mve::TriangleMesh::VertexList const& verts(mesh->get_vertices());
    mve::TriangleMesh::FaceList const& faces(mesh->get_faces());
    mve::TriangleMesh::NormalList const& vnormals(mesh->get_vertex_normals());
    mve::TriangleMesh::ColorList const& vcolors(mesh->get_vertex_colors());
    mve::TriangleMesh::TexCoordList const& vtexuv(mesh->get_vertex_texcoords());

    /* Init vertex VBO. */
    {
        VertexBuffer::Ptr vbo = VertexBuffer::create();
        vbo->set_data(&verts[0][0], verts.size(), 3);
        this->set_vertex_vbo(vbo);
    }

    /* Init index VBO if faces are given. */
    if (!faces.empty())
    {
        VertexBuffer::Ptr vbo = ogl::VertexBuffer::create();
        vbo->set_indices(&faces[0], faces.size());
        this->set_index_vbo(vbo);
    }

    /* Init normal VBO if normals are given. */
    if (!vnormals.empty())
    {
        VertexBuffer::Ptr vbo = ogl::VertexBuffer::create();
        vbo->set_data(&vnormals[0][0], vnormals.size(), 3);
        this->add_vbo(vbo, OGL_ATTRIB_NORMAL);
    }

    /* Init color VBO if colors are given. */
    if (!vcolors.empty())
    {
        VertexBuffer::Ptr vbo = ogl::VertexBuffer::create();
        vbo->set_data(&vcolors[0][0], vcolors.size(), 4);
        this->add_vbo(vbo, OGL_ATTRIB_COLOR);
    }

    /* Init UV VBO if texture coordinates are given. */
    if (!vtexuv.empty())
    {
        VertexBuffer::Ptr vbo = ogl::VertexBuffer::create();
        vbo->set_data(&vtexuv[0][0], vtexuv.size(), 2);
        this->add_vbo(vbo, OGL_ATTRIB_TEXCOORD);
    }
}

OGL_NAMESPACE_END
