#include <vector>
#include <stdexcept>
#include <algorithm>
#include <list>
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
mesh_merge (TriangleMesh::ConstPtr mesh1, TriangleMesh::Ptr mesh2)
{
    mve::TriangleMesh::VertexList const& verts1 = mesh1->get_vertices();
    mve::TriangleMesh::VertexList& verts2 = mesh2->get_vertices();
    mve::TriangleMesh::ColorList const& color1 = mesh1->get_vertex_colors();
    mve::TriangleMesh::ColorList& color2 = mesh2->get_vertex_colors();
    mve::TriangleMesh::ConfidenceList const& confs1 = mesh1->get_vertex_confidences();
    mve::TriangleMesh::ConfidenceList& confs2 = mesh2->get_vertex_confidences();
    mve::TriangleMesh::ValueList const& values1 = mesh1->get_vertex_values();
    mve::TriangleMesh::ValueList& values2 = mesh2->get_vertex_values();
    mve::TriangleMesh::NormalList const& vnorm1 = mesh1->get_vertex_normals();
    mve::TriangleMesh::NormalList& vnorm2 = mesh2->get_vertex_normals();
    mve::TriangleMesh::TexCoordList const& vtex1 = mesh1->get_vertex_texcoords();
    mve::TriangleMesh::TexCoordList& vtex2 = mesh2->get_vertex_texcoords();
    mve::TriangleMesh::NormalList const& fnorm1 = mesh1->get_face_normals();
    mve::TriangleMesh::NormalList& fnorm2 = mesh2->get_face_normals();
    mve::TriangleMesh::FaceList const& faces1 = mesh1->get_faces();
    mve::TriangleMesh::FaceList& faces2 = mesh2->get_faces();

    verts2.reserve(verts1.size() + verts2.size());
    color2.reserve(color1.size() + color2.size());
    confs2.reserve(confs1.size() + confs2.size());
    values2.reserve(values1.size() + values2.size());
    vnorm2.reserve(vnorm1.size() + vnorm2.size());
    vtex2.reserve(vtex1.size() + vtex2.size());
    fnorm2.reserve(fnorm1.size() + fnorm2.size());
    faces2.reserve(faces1.size() + faces2.size());

    std::size_t const offset = verts2.size();
    verts2.insert(verts2.end(), verts1.begin(), verts1.end());
    color2.insert(color2.end(), color1.begin(), color1.end());
    confs2.insert(confs2.end(), confs1.begin(), confs1.end());
    values2.insert(values2.end(), values1.begin(), values1.end());
    vnorm2.insert(vnorm2.end(), vnorm1.begin(), vnorm1.end());
    vtex2.insert(vtex2.end(), vtex1.begin(), vtex1.end());
    fnorm2.insert(fnorm2.end(), fnorm1.begin(), fnorm1.end());

    for (std::size_t i = 0; i < faces1.size(); ++i)
        faces2.push_back(faces1[i] + offset);
}

/* ---------------------------------------------------------------- */

mve::TriangleMesh::Ptr
mesh_components (TriangleMesh::ConstPtr mesh, std::size_t vertex_threshold)
{
    TriangleMesh::FaceList const& faces = mesh->get_faces();
    TriangleMesh::VertexList const& verts = mesh->get_vertices();
    VertexInfoList vinfos(mesh);

    std::vector<int> component_per_vertex(verts.size(), -1);
    std::size_t current_component = 0;
    for (std::size_t i = 0; i < verts.size(); ++i)
    {
        /* Start with a vertex that has no component yet. */
        if (component_per_vertex[i] >= 0)
            continue;

        /* Starting from this face, collect faces and assign component ID. */
        std::list<std::size_t> queue;
        queue.push_back(i);
        while (!queue.empty())
        {
            std::size_t vid = queue.back();
            queue.pop_back();

            /* Skip vertices with a componenet already assigned. */
            if (component_per_vertex[vid] >= 0)
                continue;
            component_per_vertex[vid] = current_component;

            /* Add all adjecent vertices to queue. */
            MeshVertexInfo::VertexRefList const& adj_verts = vinfos[vid].verts;
            queue.insert(queue.begin(), adj_verts.begin(), adj_verts.end());
        }
        current_component += 1;
    }
    vinfos.clear();

    std::vector<std::size_t> components_size(current_component, 0);
    for (std::size_t i = 0; i < component_per_vertex.size(); ++i)
        components_size[component_per_vertex[i]] += 1;

    TriangleMesh::DeleteList delete_list(verts.size(), false);
    for (std::size_t i = 0; i < component_per_vertex.size(); ++i)
        if (components_size[component_per_vertex[i]] <= vertex_threshold)
            delete_list[i] = true;

    /* Create output mesh, delete vertices, fix face indices. */
    TriangleMesh::Ptr out_mesh = mesh->duplicate();
    {
        TriangleMesh::FaceList& out_faces = out_mesh->get_faces();
        TriangleMesh::VertexList const& out_verts = out_mesh->get_vertices();

        /* Fill delete list and shift list. */
        std::size_t deleted = 0;
        std::vector<std::size_t> idxshift;
        idxshift.resize(out_verts.size());
        for (std::size_t i = 0; i < out_verts.size(); ++i)
        {
            idxshift[i] = deleted;
            if (delete_list[i] == true)
                deleted += 1;
        }

        /* Create output triangles, fixing indices. */
        out_faces.clear();
        for (std::size_t i = 0; i < faces.size(); i += 3)
        {
            if (delete_list[faces[i + 0]]
                || delete_list[faces[i + 1]]
                || delete_list[faces[i + 2]])
                continue;
            for (std::size_t j = 0; j < 3; ++j)
                out_faces.push_back(faces[i + j] - idxshift[faces[i + j]]);
        }
    }
    /* Delete vertices. */
    out_mesh->delete_vertices(delete_list);
    return out_mesh;
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
