#include <vector>
#include <stdexcept>
#include <algorithm>
#include <limits>
#include <fstream>
#include <cerrno>
#include <cstring>

#include "util/exception.h"
#include "util/string.h"
#include "math/algo.h"
#include "math/vector.h"
#include "mve/offfile.h"
#include "mve/plyfile.h"
#include "mve/pbrtfile.h"
#include "mve/vertexinfo.h"
#include "mve/meshtools.h"

MVE_NAMESPACE_BEGIN
MVE_GEOM_NAMESPACE_BEGIN

TriangleMesh::Ptr
load_mesh (std::string const& filename)
{
    if (util::string::right(filename, 4) == ".off")
        return load_off_mesh(filename);
    else if (util::string::right(filename, 4) == ".ply")
        return load_ply_mesh(filename);
    else if (util::string::right(filename, 5) == ".npts")
        return load_npts_mesh(filename, false);
    else if (util::string::right(filename, 6) == ".bnpts")
        return load_npts_mesh(filename, true);
    else
        throw std::runtime_error("Extension not recognized");
}

/* ---------------------------------------------------------------- */

void
save_mesh (TriangleMesh::ConstPtr mesh, std::string const& filename)
{
    if (util::string::right(filename, 4) == ".off")
        save_off_mesh(mesh, filename);
    else if (util::string::right(filename, 4) == ".ply")
        save_ply_mesh(mesh, filename);
    else if (util::string::right(filename, 5) == ".pbrt")
        save_pbrt_mesh(mesh, filename);
    else if (util::string::right(filename, 5) == ".npts")
        save_npts_mesh(mesh, filename, false);
    else if (util::string::right(filename, 5) == ".bnpts")
        save_npts_mesh(mesh, filename, true);
    else
        throw std::runtime_error("Extension not recognized");
}

/* ---------------------------------------------------------------- */

TriangleMesh::Ptr
load_npts_mesh (std::string const& filename, bool format_binary)
{
    /* Precondition checks. */
    if (filename.empty())
        throw std::invalid_argument("No filename given");

    /* Open file. */
    std::ifstream input(filename.c_str());
    if (!input.good())
        throw util::FileException(filename, std::strerror(errno));

    TriangleMesh::Ptr mesh = TriangleMesh::create();
    TriangleMesh::VertexList& verts = mesh->get_vertices();
    TriangleMesh::NormalList& vnorm = mesh->get_vertex_normals();

    while (true)
    {
        math::Vec3f v, n;
        if (format_binary)
        {
            input.read((char*)*v, sizeof(float) * 3);
            input.read((char*)*n, sizeof(float) * 3);
        }
        else
        {
            for (int i = 0; i < 3; ++i)
                input >> v[i];
            for (int i = 0; i < 3; ++i)
                input >> n[i];
        }
        if (input.eof())
            break;

        verts.push_back(v);
        vnorm.push_back(n);
    }

    return mesh;
}

/* ---------------------------------------------------------------- */

void
save_npts_mesh (TriangleMesh::ConstPtr mesh,
    std::string const& filename, bool format_binary)
{
    if (mesh == NULL || mesh->get_vertices().empty())
        throw std::invalid_argument("Input mesh is empty");
    if (filename.empty())
        throw std::invalid_argument("No filename given");
    if (mesh->get_vertex_normals().size() != mesh->get_vertices().size())
        throw std::invalid_argument("No vertex normals given");

    std::ofstream out(filename.c_str());
    if (!out.good())
        throw util::FileException(filename, std::strerror(errno));

    TriangleMesh::VertexList const& verts(mesh->get_vertices());
    TriangleMesh::NormalList const& vnorm(mesh->get_vertex_normals());
    for (std::size_t i = 0; i < verts.size(); ++i)
    {
        math::Vec3f const& v(verts[i]);
        math::Vec3f const& n(vnorm[i]);
        if (format_binary)
        {
            out.write((char const*)*v, sizeof(float) * 3);
            out.write((char const*)*n, sizeof(float) * 3);
        }
        else
        {
            out << v[0] << " " << v[1] << " " << v[2] << " "
                << n[0] << " " << n[1] << " " << n[2] << std::endl;
        }
    }
    out.close();
}

/* ---------------------------------------------------------------- */

/** for-each functor: homogenous matrix-vector multiplication. */
template <typename T, int D>
struct foreach_hmatrix_mult
{
    typedef math::Matrix<T,D,D> MatrixType;
    typedef math::Vector<T,D-1> VectorType;
    typedef T ValueType;

    MatrixType mat;
    ValueType val;

    foreach_hmatrix_mult (MatrixType const& matrix, ValueType const& value)
        : mat(matrix), val(value) {}
    void operator() (VectorType& vec) { vec = mat.mult(vec, val); }
};

/* ---------------------------------------------------------------- */

void
mesh_transform (mve::TriangleMesh::Ptr mesh, math::Matrix3f const& rot)
{
    if (mesh == NULL)
        throw std::invalid_argument("NULL mesh given");

    mve::TriangleMesh::VertexList& verts(mesh->get_vertices());
    mve::TriangleMesh::NormalList& vnorm(mesh->get_vertex_normals());
    mve::TriangleMesh::NormalList& fnorm(mesh->get_face_normals());

    math::algo::foreach_matrix_mult<math::Matrix3f, math::Vec3f> func(rot);
    std::for_each(verts.begin(), verts.end(), func);
    std::for_each(fnorm.begin(), fnorm.end(), func);
    std::for_each(vnorm.begin(), vnorm.end(), func);
}

/* ---------------------------------------------------------------- */

void
mesh_transform (TriangleMesh::Ptr mesh, math::Matrix4f const& trans)
{
    if (mesh == NULL)
        throw std::invalid_argument("NULL mesh given");

    mve::TriangleMesh::VertexList& verts(mesh->get_vertices());
    mve::TriangleMesh::NormalList& vnorm(mesh->get_vertex_normals());
    mve::TriangleMesh::NormalList& fnorm(mesh->get_face_normals());

    foreach_hmatrix_mult<float, 4> vfunc(trans, 1.0f);
    foreach_hmatrix_mult<float, 4> nfunc(trans, 0.0f);
    std::for_each(verts.begin(), verts.end(), vfunc);
    std::for_each(fnorm.begin(), fnorm.end(), nfunc);
    std::for_each(vnorm.begin(), vnorm.end(), nfunc);
}

/* ---------------------------------------------------------------- */

void
mesh_scale_and_center (TriangleMesh::Ptr mesh, bool scale, bool center)
{
    if (mesh == NULL)
        throw std::invalid_argument("NULL mesh given");

    TriangleMesh::VertexList& verts(mesh->get_vertices());

    if (verts.empty() || (!scale && !center))
        return;

    /* Calculate the AABB of the model. */
    math::Vec3f min(std::numeric_limits<float>::max());
    math::Vec3f max(-std::numeric_limits<float>::max());
    for (std::size_t i = 0; i < verts.size(); ++i)
        for (std::size_t j = 0; j < 3; ++j)
        {
            min[j] = std::min(min[j], verts[i][j]);
            max[j] = std::max(max[j], verts[i][j]);
        }

    math::Vec3f move((min + max) / 2.0);

    /* Prepare scaling of model to fit in the einheits cube. */
    math::Vec3f sizes(max - min);
    float max_xyz = sizes.maximum();

    /* Translate and scale. */
    if (scale && center)
    {
        for (std::size_t i = 0; i < verts.size(); ++i)
            verts[i] = (verts[i] - move) / max_xyz;
    }
    else if (scale && !center)
    {
        for (std::size_t i = 0; i < verts.size(); ++i)
            verts[i] /= max_xyz;
    }
    else if (!scale && center)
    {
        for (std::size_t i = 0; i < verts.size(); ++i)
            verts[i] -= move;
    }
}

/* ---------------------------------------------------------------- */

void
mesh_invert_faces (TriangleMesh::Ptr mesh)
{
    if (mesh == NULL)
        throw std::invalid_argument("NULL mesh given");

    TriangleMesh::FaceList& faces(mesh->get_faces());

    for (std::size_t i = 0; i < faces.size(); i += 3)
        std::swap(faces[i + 1], faces[i + 2]);
    mesh->recalc_normals(true, true);
}

/* ---------------------------------------------------------------- */

void
mesh_find_aabb (TriangleMesh::ConstPtr mesh,
    math::Vec3f& aabb_min, math::Vec3f& aabb_max)
{
    if (mesh == NULL)
        throw std::invalid_argument("NULL mesh given");

    TriangleMesh::VertexList const& verts(mesh->get_vertices());
    if (verts.empty())
        throw std::invalid_argument("Mesh without vertices given");

    aabb_min = math::Vec3f(std::numeric_limits<float>::max());
    aabb_max = math::Vec3f(-std::numeric_limits<float>::max());
    for (std::size_t i = 0; i < verts.size(); ++i)
        for (int j = 0; j < 3; ++j)
        {
            if (verts[i][j] < aabb_min[j])
                aabb_min[j] = verts[i][j];
            if (verts[i][j] > aabb_max[j])
                aabb_max[j] = verts[i][j];
        }
}

/* ---------------------------------------------------------------- */

std::size_t
mesh_delete_unreferenced (TriangleMesh::Ptr mesh)
{
    if (mesh == NULL)
        throw std::invalid_argument("NULL mesh given");

    /* Create vertex info and some short hands. */
    VertexInfoList::Ptr vinfo(VertexInfoList::create(mesh));
    TriangleMesh::VertexList const& verts(mesh->get_vertices());
    TriangleMesh::FaceList& faces(mesh->get_faces());

    /* Each vertex is shifted to the left. This list tracks the distance. */
    std::vector<std::size_t> idxshift;
    idxshift.resize(verts.size());

    /* The delete list tracks which vertices are to be deleted. */
    TriangleMesh::DeleteList dlist;
    dlist.resize(verts.size(), false);

    /* Fill delete list and shift list. */
    std::size_t deleted = 0;
    for (std::size_t i = 0; i < verts.size(); ++i)
    {
        idxshift[i] = deleted;

        if (vinfo->at(i).vclass == VERTEX_CLASS_UNREF)
        {
            dlist[i] = true;
            deleted += 1;
        }
    }

    /* Repair face list. */
    for (std::size_t i = 0; i < faces.size(); ++i)
        faces[i] -= idxshift[faces[i]];

    /* Delete vertices. */
    mesh->delete_vertices(dlist);

    return deleted;
}

MVE_GEOM_NAMESPACE_END
MVE_NAMESPACE_END
