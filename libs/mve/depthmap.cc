/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <algorithm>
#include <vector>
#include <list>
#include <set>

#include "math/defines.h"
#include "math/matrix.h"
#include "mve/mesh_info.h"
#include "mve/depthmap.h"
#include "mve/mesh_tools.h"

MVE_NAMESPACE_BEGIN
MVE_IMAGE_NAMESPACE_BEGIN

void
depthmap_cleanup_grow (FloatImage::ConstPtr dm, FloatImage::Ptr ret,
    std::vector<bool>& visited,
    std::size_t x, std::size_t y, std::size_t thres)
{
    typedef std::list<std::size_t> PixelQueue;
    typedef std::set<std::size_t> PixelBag;

    std::size_t w = dm->width();
    std::size_t h = dm->height();
    std::size_t idx = y * w + x;
    std::size_t max_idx = w * h;
    std::size_t const MAX_ST = (std::size_t)-1;

    PixelQueue queue;
    PixelBag collected;

    queue.push_front(idx);
    collected.insert(idx);

    /* Process queue of pixel until either thres is reached or no more
     * pixels are available for growing. If thres is reached, mark as
     * visited and leave pixels. If no more pixels are available, an
     * isolated small island has been found and pixels are set to zero.
     */
    while (!queue.empty())
    {
        /* Take queue element and process... */
        std::size_t cur = queue.back();
        queue.pop_back();

        /* Define 4-neighborhood for region growing. Y-1, Y+1, X-1, X+1. */
        std::size_t n[4];
        n[0] = (cur < w ? MAX_ST : cur - w);
        n[1] = (cur < max_idx - w ? cur + w : MAX_ST);
        n[2] = (cur % w > 0 ? cur - 1 : MAX_ST);
        n[3] = (cur % w < w - 1 ? cur + 1 : MAX_ST);

        /* Invalidate unreconstructed neighbors. */
        for (int i = 0; i < 4; ++i)
            if (n[i] != MAX_ST && dm->at(n[i], 0) == 0.0f)
                n[i] = MAX_ST;

        /* Add uncollected pixels to queue. */
        for (int i = 0; i < 4; ++i)
            if (n[i] != MAX_ST && collected.find(n[i]) == collected.end())
            {
                queue.push_front(n[i]);
                collected.insert(n[i]);
            }
    }

    /* Mark collected region as visited and set depth values to zero
     * if amount of collected pixels is less than threshold.
     */
    for (PixelBag::iterator i = collected.begin(); i != collected.end(); i++)
    {
        visited[*i] = true;
        if (collected.size() < thres)
            ret->at(*i, 0) = 0.0f;
    }
}

/* ---------------------------------------------------------------- */

FloatImage::Ptr
depthmap_cleanup (FloatImage::ConstPtr dm, std::size_t thres)
{
    FloatImage::Ptr ret(FloatImage::create(*dm));

    int const width = ret->width();
    int const height = ret->height();
    std::vector<bool> visited;
    visited.resize(width * height, false);

    int i = 0;
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x, ++i)
        {
            if (dm->at(i, 0) == 0.0f)
                continue;
            if (visited[i])
                continue;
            depthmap_cleanup_grow(dm, ret, visited, x, y, thres);
        }

    return ret;
}

/* ---------------------------------------------------------------- */

void
depthmap_confidence_clean (FloatImage::Ptr dm, FloatImage::ConstPtr cm)
{
    if (dm == nullptr || cm == nullptr)
        throw std::invalid_argument("Null depth or confidence map");

    if (dm->width() != cm->width() || dm->height() != cm->height())
        throw std::invalid_argument("Image dimensions do not match");

    int cnt = dm->get_pixel_amount();
    for (int i = 0; i < cnt; ++i)
        if (cm->at(i, 0) <= 0.0f)
            dm->at(i, 0) = 0.0f;
}

MVE_IMAGE_NAMESPACE_END
MVE_NAMESPACE_END

/* ---------------------------------------------------------------- */

MVE_NAMESPACE_BEGIN
MVE_GEOM_NAMESPACE_BEGIN

float
pixel_footprint (std::size_t x, std::size_t y, float depth,
    math::Matrix3f const& invproj)
{
    math::Vec3f v = invproj * math::Vec3f
        ((float)x + 0.5f, (float)y + 0.5f, 1.0f);
    return invproj[0] * depth / v.norm();
}

/* ---------------------------------------------------------------- */

math::Vec3f
pixel_3dpos (std::size_t x, std::size_t y, float depth,
    math::Matrix3f const& invproj)
{
    math::Vec3f ray = invproj * math::Vec3f
        ((float)x + 0.5f, (float)y + 0.5f, 1.0f);
    return ray.normalized() * depth;
}

/* ---------------------------------------------------------------- */

void
dm_make_triangle (TriangleMesh* mesh, mve::Image<unsigned int>& vidx,
    FloatImage const* dm, math::Matrix3f const& invproj,
    std::size_t i, int* tverts)
{
    int const width = vidx.width();
    //int const height = vidx.height();
    mve::TriangleMesh::VertexList& verts(mesh->get_vertices());
    mve::TriangleMesh::FaceList& faces(mesh->get_faces());

    for (int j = 0; j < 3; ++j)
    {
        int iidx = i + (tverts[j] % 2) + width * (tverts[j] / 2);
        int x = iidx % width;
        int y = iidx / width;
        if (vidx.at(iidx) == MATH_MAX_UINT)
        {
            /* Add vertex for depth pixel. */
            vidx.at(iidx) = verts.size();
            float depth = dm->at(iidx, 0);
            verts.push_back(pixel_3dpos(x, y, depth, invproj));
        }
        faces.push_back(vidx.at(iidx));
    }
}

/* ---------------------------------------------------------------- */

bool
dm_is_depthdisc (float* widths, float* depths, float dd_factor, int i1, int i2)
{
    /* Find index that corresponds to smaller depth. */
    int i_min = i1;
    int i_max = i2;
    if (depths[i2] < depths[i1])
        std::swap(i_min, i_max);

    /* Check if indices are a diagonal. */
    if (i1 + i2 == 3)
        dd_factor *= MATH_SQRT2;

    /* Check for depth discontinuity. */
    if (depths[i_max] - depths[i_min] > widths[i_min] * dd_factor)
        return true;

    return false;
}

/* ---------------------------------------------------------------- */

TriangleMesh::Ptr
depthmap_triangulate (FloatImage::ConstPtr dm, math::Matrix3f const& invproj,
    float dd_factor, mve::Image<unsigned int>* vids)
{
    if (dm == nullptr)
        throw std::invalid_argument("Null depthmap given");

    int const width = dm->width();
    int const height = dm->height();

    /* Prepare triangle mesh. */
    TriangleMesh::Ptr mesh(TriangleMesh::create());

    /* Generate image that maps image pixels to vertex IDs. */
    mve::Image<unsigned int> vidx(width, height, 1);
    vidx.fill(MATH_MAX_UINT);

    /* Iterate over 2x2-blocks in the depthmap and create triangles. */
    int i = 0;
    for (int y = 0; y < height - 1; ++y, ++i)
    {
        for (int x = 0; x < width - 1; ++x, ++i)
        {
            /* Cache the four depth values. */
            float depths[4] = { dm->at(i, 0), dm->at(i + 1, 0),
                dm->at(i + width, 0), dm->at(i + width + 1, 0) };

            /* Create a mask representation of the available depth values. */
            int mask = 0;
            int pixels = 0;
            for (int j = 0; j < 4; ++j)
                if (depths[j] > 0.0f)
                {
                    mask |= 1 << j;
                    pixels += 1;
                }

            /* At least three valid depth values are required. */
            if (pixels < 3)
                continue;

            /* Possible triangles, vertex indices relative to 2x2 block. */
            int tris[4][3] = {
                { 0, 2, 1 }, { 0, 3, 1 }, { 0, 2, 3 }, { 1, 2, 3 }
            };

            /* Decide which triangles to issue. */
            int tri[2] = { 0, 0 };

            switch (mask)
            {
                case 7: tri[0] = 1; break;
                case 11: tri[0] = 2; break;
                case 13: tri[0] = 3; break;
                case 14: tri[0] = 4; break;
                case 15:
                {
                    /* Choose the triangulation with smaller diagonal. */
                    float ddiff1 = std::abs(depths[0] - depths[3]);
                    float ddiff2 = std::abs(depths[1] - depths[2]);
                    if (ddiff1 < ddiff2)
                    { tri[0] = 2; tri[1] = 3; }
                    else
                    { tri[0] = 1; tri[1] = 4; }
                    break;
                }
                default: continue;
            }

            /* Omit depth discontinuity detection if dd_factor is zero. */
            if (dd_factor > 0.0f)
            {
                /* Cache pixel footprints. */
                float widths[4];
                for (int j = 0; j < 4; ++j)
                {
                    if (depths[j] == 0.0f)
                        continue;
                    widths[j] = pixel_footprint(x + (j % 2), y + (j / 2),
                        depths[j], invproj);// w, h, focal_len);
                }

                /* Check for depth discontinuities. */
                for (int j = 0; j < 2 && tri[j] != 0; ++j)
                {
                    int* tv = tris[tri[j] - 1];
                    #define DM_DD_ARGS widths, depths, dd_factor
                    if (dm_is_depthdisc(DM_DD_ARGS, tv[0], tv[1])) tri[j] = 0;
                    if (dm_is_depthdisc(DM_DD_ARGS, tv[1], tv[2])) tri[j] = 0;
                    if (dm_is_depthdisc(DM_DD_ARGS, tv[2], tv[0])) tri[j] = 0;
                }
            }

            /* Build triangles. */
            for (int j = 0; j < 2; ++j)
            {
                if (tri[j] == 0) continue;
                #define DM_MAKE_TRI_ARGS mesh.get(), vidx, dm.get(), invproj, i
                dm_make_triangle(DM_MAKE_TRI_ARGS, tris[tri[j] - 1]);
            }
        }
    }

    /* Provide the vertex ID mapping if requested. */
    if (vids != nullptr)
        std::swap(vidx, *vids);

    return mesh;
}

/* ---------------------------------------------------------------- */
TriangleMesh::Ptr
depthmap_triangulate (FloatImage::ConstPtr dm, ByteImage::ConstPtr ci,
    math::Matrix3f const& invproj, float dd_factor)
{
    if (dm == nullptr)
        throw std::invalid_argument("Null depthmap given");

    int const width = dm->width();
    int const height = dm->height();

    if (ci != nullptr && (ci->width() != width || ci->height() != height))
        throw std::invalid_argument("Color image dimension mismatch");

    /* Triangulate depth map. */
    mve::Image<unsigned int> vids;
    mve::TriangleMesh::Ptr mesh;
    mesh = mve::geom::depthmap_triangulate(dm, invproj, dd_factor, &vids);

    if (ci == nullptr)
        return mesh;

    /* Use vertex index mapping to color the mesh. */
    mve::TriangleMesh::ColorList& colors(mesh->get_vertex_colors());
    mve::TriangleMesh::VertexList const& verts(mesh->get_vertices());
    colors.resize(verts.size());

    int num_pixel = vids.get_pixel_amount();
    for (int i = 0; i < num_pixel; ++i)
    {
        if (vids[i] == MATH_MAX_UINT)
            continue;

        math::Vec4f color(ci->at(i, 0), 0.0f, 0.0f, 255.0f);
        if (ci->channels() >= 3)
        {
            color[1] = ci->at(i, 1);
            color[2] = ci->at(i, 2);
        }
        else
        {
            color[1] = color[2] = color[0];
        }
        colors[vids[i]] = color / 255.0f;
    }

    return mesh;
}

/* ---------------------------------------------------------------- */

TriangleMesh::Ptr
depthmap_triangulate (FloatImage::ConstPtr dm, ByteImage::ConstPtr ci,
    CameraInfo const& cam, float dd_factor)
{
    if (dm == nullptr)
        throw std::invalid_argument("Null depthmap given");
    if (cam.flen == 0.0f)
        throw std::invalid_argument("Invalid camera given");

    /* Triangulate depth map. */
    math::Matrix3f invproj;
    cam.fill_inverse_calibration(*invproj, dm->width(), dm->height());
    mve::TriangleMesh::Ptr mesh;
    mesh = mve::geom::depthmap_triangulate(dm, ci, invproj, dd_factor);

    /* Transform mesh to world coordinates. */
    math::Matrix4f ctw;
    cam.fill_cam_to_world(*ctw);
    mve::geom::mesh_transform(mesh, ctw);
    mesh->recalc_normals(false, true); // Remove this?

    return mesh;
}

/* ---------------------------------------------------------------- */

bool
dm_is_depth_disc (math::Vec3f const& v1,
    math::Vec3f const& v2, math::Vec3f const& v3)
{
    float const angle_threshold = MATH_DEG2RAD(15.0f);
    /* Depth discontinuity detection based on minimal angle in triangle. */
    math::Vec3f e[3] = { (v2 - v1).normalized(),
        (v3 - v2).normalized(), (v1 - v3).normalized() };
    float min_angle = angle_threshold;
    for (int i = 0; i < 3; ++i)
        min_angle = std::min(min_angle, std::acos(e[i].dot(-e[(i + 1) % 3])));
    //std::cout << "Min angle is " << MATH_RAD2DEG(min_angle) << ", "
    //    << (min_angle < ANGLE_THRES) << std::endl;
    return min_angle < angle_threshold;
}

void
rangegrid_triangulate (Image<unsigned int> const& grid, TriangleMesh::Ptr mesh)
{
    if (mesh == nullptr)
        throw std::invalid_argument("Null mesh given");

    int w = grid.width();
    int h = grid.height();

    TriangleMesh::VertexList const& verts(mesh->get_vertices());
    TriangleMesh::FaceList& faces(mesh->get_faces());

    for (int y = 0; y < h - 1; ++y)
        for (int x = 0; x < w - 1; ++x)
        {
            int i = y * w + x;

            unsigned int vid[4] = { grid[i], grid[i + 1],
                grid[i + w], grid[i + w + 1] };

            /* Create a mask representation of the available indices. */
            int mask = 0;
            int indices = 0;
            for (int j = 0; j < 4; ++j)
                if (vid[j] != static_cast<unsigned int>(-1))
                {
                    mask |= 1 << j;
                    indices += 1;
                }

            /* At least three valid depth values are required. */
            if (indices < 3)
                continue;

            /* Decide which triangles to issue.
             * Triangle 1: (0,2,1), 2: (0,3,1), 3: (0,2,3), 4: (1,2,3) */
            int tri[2] = { 0, 0 };

            switch (mask)
            {
                case 7: tri[0] = 1; break;
                case 11: tri[0] = 2; break;
                case 13: tri[0] = 3; break;
                case 14: tri[0] = 4; break;
                case 15:
                {
                    /* Choose the triangulation with smaller diagonal. */
                    if ((verts[vid[0]] - verts[vid[3]]).square_norm() <
                        (verts[vid[1]] - verts[vid[2]]).square_norm())
                    { tri[0] = 2; tri[1] = 3; }
                    else
                    { tri[0] = 1; tri[1] = 4; }
                    break;
                }
                default: continue;
            }

            /* Check for depth discontinuities and issue triangles. */
            for (int j = 0; j < 2; ++j)
            {
                switch (tri[j])
                {
#define VERT(id) verts[vid[id]]
#define DEPTHDISC(a,b,c) dm_is_depth_disc(VERT(a), VERT(b), VERT(c))
#define ADDTRI(a,c,b) faces.push_back(vid[a]); faces.push_back(vid[b]); faces.push_back(vid[c])
                case 1: if (!DEPTHDISC(0,2,1)) { ADDTRI(0,2,1); } break;
                case 2: if (!DEPTHDISC(0,3,1)) { ADDTRI(0,3,1); } break;
                case 3: if (!DEPTHDISC(0,2,3)) { ADDTRI(0,2,3); } break;
                case 4: if (!DEPTHDISC(1,2,3)) { ADDTRI(1,2,3); } break;
                default: break;
                }
            }
        }
}

/* ---------------------------------------------------------------- */

void
depthmap_mesh_confidences (TriangleMesh::Ptr mesh, int iterations)
{
    if (mesh == nullptr)
        throw std::invalid_argument("Null mesh given");

    if (iterations < 0)
        throw std::invalid_argument("Invalid amount of iterations");

    if (iterations == 0)
        return;

    TriangleMesh::ConfidenceList& confs(mesh->get_vertex_confidences());
    TriangleMesh::VertexList const& verts(mesh->get_vertices());
    confs.clear();
    confs.resize(verts.size(), 1.0f);

    /* Find boundary vertices and remember them. */
    std::vector<std::size_t> vidx;
    MeshInfo mesh_info(mesh);

    for (std::size_t i = 0; i < mesh_info.size(); ++i)
    {
        if (mesh_info[i].vclass == MeshInfo::VERTEX_CLASS_BORDER)
            vidx.push_back(i);
    }

    /* Iteratively expand the current region and update confidences. */
    for (int current = 0; current < iterations; ++current)
    {
        /* Calculate confidence for that iteration. */
        float conf = (float)current / (float)iterations;
        //conf = math::algo::fastpow(conf, 3);

        /* Assign current confidence to all vertices. */
        for (std::size_t i = 0; i < vidx.size(); ++i)
            confs[vidx[i]] = conf;

        /* Replace vertex list with adjacent vertices. */
        std::vector<std::size_t> cvidx;
        std::swap(vidx, cvidx);
        for (std::size_t i = 0; i < cvidx.size(); ++i)
        {
            MeshInfo::VertexInfo info = mesh_info[cvidx[i]];
            for (std::size_t j = 0; j < info.verts.size(); ++j)
                if (confs[info.verts[j]] == 1.0f)
                    vidx.push_back(info.verts[j]);
        }
    }
}

/* ---------------------------------------------------------------- */

void
depthmap_mesh_peeling (TriangleMesh::Ptr mesh, int iterations)
{
    if (mesh == nullptr)
        throw std::invalid_argument("Null mesh given");

    if (iterations < 0)
        throw std::invalid_argument("Invalid amount of iterations");

    if (iterations == 0)
        return;

    TriangleMesh::FaceList& faces(mesh->get_faces());
    std::vector<bool> delete_list;
    delete_list.resize(faces.size(), false);

    /* Iteratively invalidate triangles at the boundary. */
    for (int iter = 0; iter < iterations; ++iter)
    {
        MeshInfo mesh_info(mesh);
        for (std::size_t i = 0; i < mesh_info.size(); ++i)
        {
            MeshInfo::VertexInfo const& info(mesh_info[i]);
            if (info.vclass == MeshInfo::VERTEX_CLASS_BORDER)
                for (std::size_t j = 0; j < info.faces.size(); ++j)
                    for (int k = 0; k < 3; ++k)
                    {
                        unsigned int fidx = info.faces[j] * 3 + k;
                        faces[fidx] = 0;
                        delete_list[fidx] = true;
                    }
        }
    }

    /* Remove invalidated faces. */
    math::algo::vector_clean(delete_list, &faces);
}

MVE_GEOM_NAMESPACE_END
MVE_NAMESPACE_END
