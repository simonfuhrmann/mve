/*
 * Triangle mesh vertex info data structure.
 * Written by Simon Fuhrmann
 */

#ifndef MVE_VERTEX_INFO_HEADER
#define MVE_VERTEX_INFO_HEADER

#include <vector>

#include "util/refptr.h"
#include "mve/defines.h"
#include "mve/trianglemesh.h"

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

private:
    void order_and_classify (TriangleMesh const& mesh, std::size_t idx);

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

    /** Prints debug information to stdout. */
    void print_debug (void);

    /** Checks for the existence of and edge between the given vertices. */
    bool is_mesh_edge (std::size_t v1, std::size_t v2);

    /** Fills the given AdjFaceList with all faces containing the edge. */
    void get_faces_for_edge (std::size_t v1, std::size_t v2,
        std::vector<std::size_t>* afaces);

    /* TODO: More helper functions
     * - get_mem_usage
     */
};

/* ------------------------- Implementation ----------------------- */

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
