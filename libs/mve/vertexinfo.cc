#include <iostream>
#include <list>
#include <algorithm>
#include <set>

#include "mve/vertexinfo.h"

MVE_NAMESPACE_BEGIN

void
VertexInfoList::calculate (TriangleMesh::ConstPtr mesh)
{
    TriangleMesh::VertexList const& verts = mesh->get_vertices();
    TriangleMesh::FaceList const& faces = mesh->get_faces();
    std::size_t face_amount = faces.size() / 3;

    this->clear();
    this->resize(verts.size());

    /* Build data structure. */
    std::size_t i3 = 0;
    for (std::size_t i = 0; i < face_amount; ++i)
        for (std::size_t j = 0; j < 3; ++j, ++i3)
            this->at(faces[i3]).faces.push_back(i);

    /* Order and classify all vertices. */
    for (std::size_t i = 0; i < this->size(); ++i)
        this->order_and_classify(*mesh, i);
}

/* ---------------------------------------------------------------- */

/* Adjacent face representation for the ordering algorithm. */
struct FaceRep
{
    std::size_t face_id;
    std::size_t first;
    std::size_t second;

    FaceRep (std::size_t fid, std::size_t first, std::size_t second)
        : face_id(fid), first(first), second(second)
    {}
};
typedef std::list<FaceRep> FaceRepList;

/* ---------------------------------------------------------------- */

void
VertexInfoList::order_and_classify (TriangleMesh const& mesh,
    std::size_t idx)
{
    TriangleMesh::FaceList const& faces(mesh.get_faces());
    MeshVertexInfo& vinfo(this->at(idx));
    MeshVertexInfo::FaceRefList& adj(vinfo.faces);

    /* Detect unreferenced vertices. */
    if (adj.empty())
    {
        vinfo.vclass = VERTEX_CLASS_UNREF;
        return;
    }

    /* Build list of FaceRep objects to sort faces. */
    FaceRepList flist;
    for (std::size_t i = 0; i < adj.size(); ++i)
    {
        std::size_t foff = adj[i] * 3;
        for (std::size_t j = 0; j < 3; ++j)
            if (faces[foff + j] == idx)
            {
                flist.push_back(FaceRep(adj[i],
                    faces[foff + (j + 1) % 3], faces[foff + (j + 2) % 3]));
                break;
            }
    }

    /* Sort faces by chaining adjacent faces in sflist. */
    FaceRepList sflist;
    sflist.push_back(flist.front());
    flist.pop_front();
    while (!flist.empty())
    {
        std::size_t front_id = sflist.front().first;
        std::size_t back_id = sflist.back().second;
        bool pushed = false;
        for (FaceRepList::iterator i = flist.begin(); i != flist.end(); ++i)
            if (i->second == front_id)
            {
                sflist.push_front(*i);
                flist.erase(i);
                pushed = true;
                break;
            }
            else if (i->first == back_id)
            {
                sflist.push_back(*i);
                flist.erase(i);
                pushed = true;
                break;
            }

        /* The vertex is complex. */
        if (!pushed)
        {
            sflist.insert(sflist.end(), flist.begin(), flist.end());
            break;
        }
    }

    /* Detect vertex class. */
    if (!flist.empty())
        vinfo.vclass = VERTEX_CLASS_COMPLEX;
    else if (sflist.front().first == sflist.back().second)
        vinfo.vclass = VERTEX_CLASS_SIMPLE;
    else
        vinfo.vclass = VERTEX_CLASS_BORDER;

    /* Insert the face IDs in the adjacent faces list. */
    vinfo.faces.clear();
    for (FaceRepList::iterator i = sflist.begin(); i != sflist.end(); ++i)
        vinfo.faces.push_back(i->face_id);

    /* Insert vertices into the adjacent vertices list. */
    vinfo.verts.clear();
    switch (vinfo.vclass)
    {
    case VERTEX_CLASS_SIMPLE:
    case VERTEX_CLASS_BORDER:
        for (FaceRepList::iterator i = sflist.begin(); i != sflist.end(); ++i)
            vinfo.verts.push_back(i->first);
        if (vinfo.vclass == VERTEX_CLASS_BORDER)
            vinfo.verts.push_back(sflist.back().second);
        break;

    case VERTEX_CLASS_COMPLEX:
        {
            std::set<std::size_t> vset;
            for (FaceRepList::iterator i = sflist.begin();
                i != sflist.end(); ++i)
            {
                vset.insert(i->first);
                vset.insert(i->second);
            }
            vinfo.verts.insert(vinfo.verts.begin(), vset.begin(), vset.end());
            break;
        }
    case VERTEX_CLASS_UNREF:
    default:
        break;
    }
}

/* ---------------------------------------------------------------- */

void
VertexInfoList::print_debug (void)
{
    for (std::size_t i = 0; i < this->size(); ++i)
    {
        MeshVertexInfo& vinfo = this->at(i);
        std::cout << "Stats for vertex " << i << ", class "
            << vinfo.vclass << std::endl;
        std::cout << "  Faces: ";
        for (std::size_t i = 0; i < vinfo.faces.size(); ++i)
            std::cout << vinfo.faces[i] << " ";
        std::cout << std::endl;

        std::cout << "  Vertices: ";
        for (std::size_t i = 0; i < vinfo.verts.size(); ++i)
            std::cout << vinfo.verts[i] << " ";
        std::cout << std::endl;
    }
}

/* ---------------------------------------------------------------- */

bool
VertexInfoList::is_mesh_edge (std::size_t v1, std::size_t v2)
{
    MeshVertexInfo::VertexRefList const& verts = this->at(v1).verts;
    return std::find(verts.begin(), verts.end(), v2) != verts.end();
}

/* ---------------------------------------------------------------- */

void
VertexInfoList::get_faces_for_edge (std::size_t v1, std::size_t v2,
    std::vector<std::size_t>* afaces)
{
    MeshVertexInfo::FaceRefList const& faces1 = this->at(v1).faces;
    MeshVertexInfo::FaceRefList const& faces2 = this->at(v2).faces;
    std::set<std::size_t> faces2_set;
    faces2_set = std::set<std::size_t>(faces2.begin(), faces2.end());
    for (std::size_t i = 0; i < faces1.size(); ++i)
    {
        if (faces2_set.find(faces1[i]) != faces2_set.end())
            afaces->push_back(faces1[i]);
    }
}

MVE_NAMESPACE_END
