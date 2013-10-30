/*
 * Triangle mesh vertex info data structure.
 * Written by Simon Fuhrmann
 */

#ifndef MVE_VERTEX_INFO_HEADER
#define MVE_VERTEX_INFO_HEADER

#include <algorithm>
#include <vector>

#include "util/ref_ptr.h"
#include "mve/defines.h"
#include "mve/mesh.h"

MVE_NAMESPACE_BEGIN

/**
 * Each vertex is classified into one of SIMPLE, COMPLEX, BORDER and
 * UNREF. A simple vertex has a full fan of adjacent triangles. A
 * border vertex has a single but incomplete fan of adjacent
 * triangles. An unreferenced vertex has no adjacent triangles.
 * Everything else is a complex vertex, which is basically a
 * non-2-manifold configuration.
 */
enum MeshVertexClass
{
    VERTEX_CLASS_SIMPLE,
    VERTEX_CLASS_COMPLEX,
    VERTEX_CLASS_BORDER,
    VERTEX_CLASS_UNREF
};

/* ---------------------------------------------------------------- */

/**
 * This class holds per-vertex information, namely the
 * vertex class, the list of adjacent faces and vertices.
 */
struct MeshVertexInfo
{
    typedef std::vector<std::size_t> FaceRefList;
    typedef std::vector<std::size_t> VertexRefList;

    MeshVertexClass vclass;
    VertexRefList verts;
    FaceRefList faces;

    void remove_adjacent_face (std::size_t face_id);
    void remove_adjacent_vertex (std::size_t vertex_id);
    void replace_adjacent_face (std::size_t old_value, std::size_t new_value);
    void replace_adjacent_vertex (std::size_t old_value, std::size_t new_value);
};

/* ---------------------------------------------------------------- */

/**
 * This class extracts per-vertex information. Each vertex is
 * cassified into one of the classes SIMPLE, COMPLEX, BORDER and
 * UNREF, see above. Adjacent faces and vertices are collected for
 * each vertex and stored in the MeshVertexInfo struct.
 */
class VertexInfoList : public std::vector<MeshVertexInfo>
{
public:
    typedef util::RefPtr<VertexInfoList> Ptr;
    typedef util::RefPtr<VertexInfoList const> ConstPtr;


public:
    /* Creates an uninitialized vertex info list. */
    VertexInfoList (void);
    /* Creates and initializes a vertex info list. */
    VertexInfoList (TriangleMesh::ConstPtr mesh);

    /** Creates an empty vertex list. */
    static Ptr create (void);

    /** Creates vertex info for the given mesh. */
    static Ptr create (TriangleMesh::ConstPtr mesh);

    /** Calculates vertex info for the given mesh. */
    void calculate (TriangleMesh::ConstPtr mesh);

    /**
     * Orders and classifies adjacent vertices and faces.
     * This is usually done by calculate() but can be called after modifying
     * the mesh data structure. The list of adjacent faces is expected
     * to be complete.
     */
    void order_and_classify (TriangleMesh const& mesh, std::size_t idx);

    /** Checks for the existence of and edge between the given vertices. */
    bool is_mesh_edge (std::size_t v1, std::size_t v2);

    /** Fills the given vector with all faces containing the edge. */
    void get_faces_for_edge (std::size_t v1, std::size_t v2,
        std::vector<std::size_t>* afaces);

    /* TODO: More helper functions
     * - get_mem_usage
     */
};

/* ------------------------- Implementation ----------------------- */

inline void
MeshVertexInfo::remove_adjacent_face (std::size_t face_id)
{
    this->faces.erase(std::remove(this->faces.begin(), this->faces.end(),
        face_id), this->faces.end());
}

inline void
MeshVertexInfo::remove_adjacent_vertex (std::size_t vertex_id)
{
    this->verts.erase(std::remove(this->verts.begin(), this->verts.end(),
        vertex_id), this->verts.end());
}

inline void
MeshVertexInfo::replace_adjacent_face (std::size_t old_value,
    std::size_t new_value)
{
    std::replace(this->faces.begin(), this->faces.end(), old_value, new_value);
}

inline void
MeshVertexInfo::replace_adjacent_vertex (std::size_t old_value,
    std::size_t new_value)
{
    std::replace(this->verts.begin(), this->verts.end(), old_value, new_value);
}

inline
VertexInfoList::VertexInfoList (void)
{
}

inline
VertexInfoList::VertexInfoList (TriangleMesh::ConstPtr mesh)
{
    this->calculate(mesh);
}

inline VertexInfoList::Ptr
VertexInfoList::create (void)
{
    return Ptr(new VertexInfoList);
}

inline VertexInfoList::Ptr
VertexInfoList::create (TriangleMesh::ConstPtr mesh)
{
    Ptr ret(new VertexInfoList);
    ret->calculate(mesh);
    return ret;
}

MVE_NAMESPACE_END

#endif /* MVE_VERTEX_INFO_HEADER */
