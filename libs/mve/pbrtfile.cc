#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cerrno>
#include <cstring>

#include "util/exception.h"
#include "mve/pbrtfile.h"

MVE_NAMESPACE_BEGIN
MVE_GEOM_NAMESPACE_BEGIN

void
save_pbrt_mesh (TriangleMesh::ConstPtr mesh, std::string const& filename)
{
    if (mesh.get() == 0)
        throw std::invalid_argument("NULL mesh given");
    if (filename.empty())
        throw std::invalid_argument("No filename given");

    mve::TriangleMesh::VertexList const& verts(mesh->get_vertices());
    mve::TriangleMesh::NormalList const& vnormals(mesh->get_vertex_normals());
    mve::TriangleMesh::FaceList const& faces(mesh->get_faces());

    /* Open output file. */
    std::ofstream out(filename.c_str());
    if (!out.good())
        throw util::FileException(filename, std::strerror(errno));

    out << "Translate 0 0 0" << std::endl;
    out << "Shape \"trianglemesh\"" << std::endl;

    /* Issue vertices. */
    out << "\"point P\" [" << std::endl;
    for (std::size_t i = 0; i < verts.size(); ++i)
    {
        out << "  "
            << verts[i][0] << " "
            << verts[i][1] << " "
            << verts[i][2] << std::endl;
    }
    out << "]" << std::endl << std::endl;

    /* Issue normals. */
    if (vnormals.size() == verts.size())
    {
        out << "\"normal N\" [" << std::endl;
        for (std::size_t i = 0; i < vnormals.size(); ++i)
        {
            out << "  "
                << vnormals[i][0] << " "
                << vnormals[i][1] << " "
                << vnormals[i][2] << std::endl;
        }
        out << "]" << std::endl << std::endl;
    }

    /* Issue face indices. */
    out << "\"integer indices\" [" << std::endl;
    for (std::size_t i = 0 ; i < faces.size() ; ++i)
    {
        if (!(i % 3))
            out << "  ";
        else
            out << " ";
        out << faces[i];
        if ((i % 3) == 2)
            out << std::endl;
    }
    out << "]" << std::endl;

    /* Close output file. */
    out.close();
}


MVE_GEOM_NAMESPACE_END
MVE_NAMESPACE_END
