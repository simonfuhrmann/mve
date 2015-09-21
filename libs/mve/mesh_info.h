/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef MVE_VERTEX_INFO_HEADER
#define MVE_VERTEX_INFO_HEADER

#include <algorithm>
#include <vector>
#include <memory>

#include "mve/defines.h"
#include "mve/mesh.h"

MVE_NAMESPACE_BEGIN

class MeshInfo
{
public:
    /** Vertex classification according to adjacent triangles. */
    enum VertexClass
    {
        /** Vertex with a single closed fan of adjacent triangles. */
        VERTEX_CLASS_SIMPLE,
        /** Vertex with a single but open fan of triangles. */
        VERTEX_CLASS_BORDER,
        /** Vertex with more than one triangle fan. */
        VERTEX_CLASS_COMPLEX,
        /** Vertiex without any adjacent triangles. */
        VERTEX_CLASS_UNREF
    };

    typedef std::vector<std::size_t> AdjacentVertices;
    typedef std::vector<std::size_t> AdjacentFaces;

    /** Per-vertex classification and adjacency information. */
    struct VertexInfo
    {
        VertexClass vclass;
        AdjacentVertices verts;
        AdjacentFaces faces;

        void remove_adjacent_face (std::size_t face_id);
        void remove_adjacent_vertex (std::size_t vertex_id);
        void replace_adjacent_face (std::size_t old_id, std::size_t new_id);
        void replace_adjacent_vertex (std::size_t old_id, std::size_t new_id);
    };

public:
    /** Constructor without initialization. */
    MeshInfo (void);

    /** Constructor with initialization for the given mesh. */
    MeshInfo (TriangleMesh::ConstPtr mesh);

    /** Initializes the data structure for the given mesh. */
    void initialize (TriangleMesh::ConstPtr mesh);

    /**
     * Updates the vertex info for a single vertex. It expects that the
     * list of adjacent faces is complete (but unorderd), and recomputes
     * adjacent face ordering, adjacent vertices and the vertex class.
     */
    void update_vertex (TriangleMesh const& mesh, std::size_t vertex_id);

    /** Checks for the existence of an edge between the given vertices. */
    bool is_mesh_edge (std::size_t v1, std::size_t v2) const;

    /** Returns faces adjacent to both vertices. */
    void get_faces_for_edge (std::size_t v1, std::size_t v2,
        std::vector<std::size_t>* adjacent_faces) const;

public:
    VertexInfo& operator[] (std::size_t id);
    VertexInfo const& operator[] (std::size_t id) const;
    VertexInfo& at (std::size_t id);
    VertexInfo const& at (std::size_t id) const;
    std::size_t size (void) const;
    void clear (void);

private:
    std::vector<VertexInfo> vertex_info;
};

/* ------------------------- Implementation ----------------------- */

inline
MeshInfo::MeshInfo (void)
{
}

inline
MeshInfo::MeshInfo (TriangleMesh::ConstPtr mesh)
{
    this->initialize(mesh);
}

inline MeshInfo::VertexInfo&
MeshInfo::operator[] (std::size_t id)
{
    return this->vertex_info[id];
}

inline MeshInfo::VertexInfo const&
MeshInfo::operator[] (std::size_t id) const
{
    return this->vertex_info[id];
}

inline MeshInfo::VertexInfo&
MeshInfo::at (std::size_t id)
{
    return this->vertex_info[id];
}

inline MeshInfo::VertexInfo const&
MeshInfo::at (std::size_t id) const
{
    return this->vertex_info[id];
}

inline std::size_t
MeshInfo::size (void) const
{
    return this->vertex_info.size();
}

inline void
MeshInfo::clear (void)
{
    std::vector<VertexInfo>().swap(this->vertex_info);
}

inline void
MeshInfo::VertexInfo::remove_adjacent_face (std::size_t face_id)
{
    this->faces.erase(std::remove(this->faces.begin(), this->faces.end(),
        face_id), this->faces.end());
}

inline void
MeshInfo::VertexInfo::remove_adjacent_vertex (std::size_t vertex_id)
{
    this->verts.erase(std::remove(this->verts.begin(), this->verts.end(),
        vertex_id), this->verts.end());
}

inline void
MeshInfo::VertexInfo::replace_adjacent_face (std::size_t old_id,
    std::size_t new_id)
{
    std::replace(this->faces.begin(), this->faces.end(), old_id, new_id);
}

inline void
MeshInfo::VertexInfo::replace_adjacent_vertex (std::size_t old_id,
    std::size_t new_id)
{
    std::replace(this->verts.begin(), this->verts.end(), old_id, new_id);
}

MVE_NAMESPACE_END

#endif /* MVE_VERTEX_INFO_HEADER */
