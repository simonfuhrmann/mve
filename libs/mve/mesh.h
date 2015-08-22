/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef MVE_TRIANGLE_MESH_HEADER
#define MVE_TRIANGLE_MESH_HEADER

#include <vector>
#include <memory>

#include "math/vector.h"
#include "mve/defines.h"

MVE_NAMESPACE_BEGIN

/**
 * Base class for meshes. This class essentially contains the vertex data
 * and vertex associated data, namely colors, confidences and the generic
 * attribute values.
 */
class MeshBase
{
public:
    typedef std::shared_ptr<MeshBase> Ptr;
    typedef std::shared_ptr<MeshBase const> ConstPtr;

    typedef unsigned int VertexID;
    typedef std::vector<math::Vec3f> VertexList;
    typedef std::vector<math::Vec4f> ColorList;
    typedef std::vector<float> ConfidenceList;
    typedef std::vector<float> ValueList;

public:
    virtual ~MeshBase (void);

    /** Returns the mesh vertices. */
    VertexList const& get_vertices (void) const;
    /** Returns the mesh vertices. */
    VertexList& get_vertices (void);

    /** Returns the vertex colors. */
    ColorList const& get_vertex_colors (void) const;
    /** Returns the vertex colors. */
    ColorList& get_vertex_colors (void);

    /** Returns the vertex confidences. */
    ConfidenceList const& get_vertex_confidences (void) const;
    /** Returns the vertex confidences. */
    ConfidenceList& get_vertex_confidences (void);

    /** Returns the vertex values (generic attribute). */
    ValueList const& get_vertex_values (void) const;
    /** Returns the vertex values (generic attribute). */
    ValueList& get_vertex_values (void);

    /** Returns true if colors and vertex amount are equal. */
    bool has_vertex_colors (void) const;
    /** Returns true if confidence amount and vertex amount are equal. */
    bool has_vertex_confidences (void) const;
    /** Returns true if value amount and vertex amount are equal. */
    bool has_vertex_values (void) const;

    /** Clears all mesh data. */
    virtual void clear (void);

protected:
    MeshBase (void);

protected:
    VertexList vertices;
    ColorList vertex_colors;
    ConfidenceList vertex_confidences;
    ValueList vertex_values;
};

/* ---------------------------------------------------------------- */

/**
 * Triangle mesh representation.
 * The triangle mesh holds a list of vertices,
 * per-vertex normals, colors, and confidences, a list
 * of vertex indices for the faces and per-face normals.
 */
class TriangleMesh : public MeshBase
{
public:
    typedef std::shared_ptr<TriangleMesh> Ptr;
    typedef std::shared_ptr<TriangleMesh const> ConstPtr;

    typedef std::vector<math::Vec3f> NormalList;
    typedef std::vector<math::Vec2f> TexCoordList;
    typedef std::vector<VertexID> FaceList;

    typedef std::vector<bool> DeleteList;

public:
    virtual ~TriangleMesh (void);
    static Ptr create (void);
    static Ptr create (TriangleMesh::ConstPtr other);

    Ptr duplicate (void) const;

    /** Returns the vertex normals. */
    NormalList const& get_vertex_normals (void) const;
    /** Returns the vertex normals. */
    NormalList& get_vertex_normals (void);

    /** Returns the vectex texture coordinates. */
    TexCoordList const& get_vertex_texcoords (void) const;
    /** Returns the vectex texture coordinates. */
    TexCoordList& get_vertex_texcoords (void);

    /** Returns the triangle indices. */
    FaceList const& get_faces (void) const;
    /** Returns the triangle indices. */
    FaceList& get_faces (void);

    /** Returns the face normals. */
    NormalList const& get_face_normals (void) const;
    /** Returns the face normals. */
    NormalList& get_face_normals (void);

    /** Returns the face colors. */
    ColorList const& get_face_colors (void) const;
    /** Returns the face colors. */
    ColorList& get_face_colors (void);

    /** Returns true if vertex normal amount equals vertex amount. */
    bool has_vertex_normals (void) const;
    /** Returns true if texture coordinate amount equals vertex amount. */
    bool has_vertex_texcoords (void) const;
    /** Returns true if face normal amount equals face amount. */
    bool has_face_normals (void) const;
    /** Returns true if face color amount equals face amount. */
    bool has_face_colors (void) const;

    /** Recalculates normals if normal amount is inconsistent. */
    void ensure_normals (bool face = true, bool vertex = true);
    /** Recalculates face and/or vertex normals. */
    void recalc_normals (bool face = true, bool vertex = true);

    /** Clears all mesh data. */
    virtual void clear (void);
    /** Clears mesh normal data. */
    void clear_normals (void);

    /**
     * Deletes marked vertices and related attributes if available.
     * Note that this does not change face data.
     */
    void delete_vertices (DeleteList const& dlist);

    /*
     * Deletes marked vertices and related attributes, deletes faces
     * referencing marked vertices and fixes face indices.
     */
    void delete_vertices_fix_faces (DeleteList const& dlist);

    /**
     * Deletes all invalid triangles, i.e. the triangles where all three
     * vertices have the same vertex ID.
     */
    void delete_invalid_faces (void);

    /** Returns the memory consumption in bytes. */
    std::size_t get_byte_size (void) const;

protected:
    NormalList vertex_normals;
    TexCoordList vertex_texcoords;

    FaceList faces;
    NormalList face_normals;
    ColorList face_colors;

protected:
    /** Use the create() methods to instantiate a mesh. */
    TriangleMesh (void);
};

/* ---------------------------------------------------------------- */

inline
MeshBase::MeshBase (void)
{
}

inline
MeshBase::~MeshBase (void)
{
}

inline MeshBase::VertexList const&
MeshBase::get_vertices (void) const
{
    return this->vertices;
}

inline MeshBase::VertexList&
MeshBase::get_vertices (void)
{
    return this->vertices;
}

inline MeshBase::ColorList const&
MeshBase::get_vertex_colors (void) const
{
    return this->vertex_colors;
}

inline MeshBase::ColorList&
MeshBase::get_vertex_colors (void)
{
    return this->vertex_colors;
}

inline MeshBase::ConfidenceList const&
MeshBase::get_vertex_confidences (void) const
{
    return this->vertex_confidences;
}

inline MeshBase::ConfidenceList&
MeshBase::get_vertex_confidences (void)
{
    return this->vertex_confidences;
}

inline MeshBase::ValueList const&
MeshBase::get_vertex_values (void) const
{
    return this->vertex_values;
}

inline MeshBase::ValueList&
MeshBase::get_vertex_values (void)
{
    return this->vertex_values;
}

inline void
MeshBase::clear (void)
{
    this->vertices.clear();
    this->vertex_colors.clear();
    this->vertex_confidences.clear();
    this->vertex_values.clear();
}

inline bool
MeshBase::has_vertex_colors (void) const
{
    return !this->vertices.empty()
        && this->vertex_colors.size() == this->vertices.size();
}

inline bool
MeshBase::has_vertex_confidences (void) const
{
    return !this->vertices.empty()
        && this->vertex_confidences.size() == this->vertices.size();
}

inline bool
MeshBase::has_vertex_values (void) const
{
    return !this->vertices.empty()
        && this->vertex_values.size() == this->vertices.size();
}

/* ---------------------------------------------------------------- */

inline TriangleMesh::Ptr
TriangleMesh::duplicate (void) const
{
    return Ptr(new TriangleMesh(*this));
}

inline
TriangleMesh::TriangleMesh (void)
{
}

inline
TriangleMesh::~TriangleMesh (void)
{
}

inline TriangleMesh::Ptr
TriangleMesh::create (void)
{
    return Ptr(new TriangleMesh);
}

inline TriangleMesh::Ptr
TriangleMesh::create (TriangleMesh::ConstPtr other)
{
    return Ptr(new TriangleMesh(*other));
}

inline TriangleMesh::NormalList const&
TriangleMesh::get_vertex_normals (void) const
{
    return this->vertex_normals;
}

inline TriangleMesh::NormalList&
TriangleMesh::get_vertex_normals (void)
{
    return this->vertex_normals;
}

inline TriangleMesh::TexCoordList const&
TriangleMesh::get_vertex_texcoords (void) const
{
    return this->vertex_texcoords;
}

inline TriangleMesh::TexCoordList&
TriangleMesh::get_vertex_texcoords (void)
{
    return this->vertex_texcoords;
}

inline TriangleMesh::FaceList const&
TriangleMesh::get_faces (void) const
{
    return this->faces;
}

inline TriangleMesh::FaceList&
TriangleMesh::get_faces (void)
{
    return this->faces;
}

inline TriangleMesh::NormalList const&
TriangleMesh::get_face_normals (void) const
{
    return this->face_normals;
}

inline TriangleMesh::NormalList&
TriangleMesh::get_face_normals (void)
{
    return this->face_normals;
}

inline TriangleMesh::ColorList&
TriangleMesh::get_face_colors (void)
{
    return this->face_colors;
}

inline TriangleMesh::ColorList const&
TriangleMesh::get_face_colors (void) const
{
    return this->face_colors;
}

inline void
TriangleMesh::clear_normals (void)
{
    this->vertex_normals.clear();
    this->face_normals.clear();
}

inline void
TriangleMesh::clear (void)
{
    this->MeshBase::clear();
    this->vertex_normals.clear();
    this->vertex_texcoords.clear();
    this->faces.clear();
    this->face_normals.clear();
    this->face_colors.clear();
}

inline bool
TriangleMesh::has_vertex_normals (void) const
{
    return !this->vertices.empty()
        && this->vertex_normals.size() == this->vertices.size();
}

inline bool
TriangleMesh::has_vertex_texcoords (void) const
{
    return !this->vertices.empty()
        && this->vertex_texcoords.size() == this->vertices.size();
}

inline bool
TriangleMesh::has_face_normals (void) const
{
    return !this->faces.empty()
        && this->faces.size() == this->face_normals.size() * 3;
}

inline bool
TriangleMesh::has_face_colors (void) const
{
    return !this->faces.empty()
        && this->faces.size() == this->face_colors.size() * 3;
}

MVE_NAMESPACE_END

#endif /* MVE_TRIANGLE_MESH_HEADER */
