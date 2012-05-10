#include <algorithm>
#include <vector>
#include <list>
#include <set>

#include "math/defines.h"
#include "math/matrix.h"

#include "vertexinfo.h"
#include "depthmap.h"
#include "meshtools.h"
#include "bilateral.h"

MVE_NAMESPACE_BEGIN
MVE_IMAGE_NAMESPACE_BEGIN

/**
 *
 */
#define DILATE_MEDIAN 0
float
dilate_depth (FloatImage::ConstPtr dm,
    std::size_t cx, std::size_t cy, std::size_t ks)
{
    std::size_t w = dm->width();
    std::size_t h = dm->height();
    std::size_t x1, x2, y1, y2;
    math::algo::kernel_region(cx, cy, ks, w, h, x1, x2, y1, y2);

#if DILATE_MEDIAN
    std::vector<float> values;
    values.reserve(ks * ks);
#else
    float total_depth = 0.0f;
    std::size_t valid_pixels = 0;
#endif
    for (std::size_t y = y1; y <= y2; ++y)
        for (std::size_t x = x1; x <= x2; ++x)
        {
            /* Skip center pixel. */
            if (x == cx && y == cy)
                continue;

            /* Remember non-zero depth values. */
            float depth = dm->at(x, y, 0);
            if (depth != 0.0f)
            {
#if DILATE_MEDIAN
                values.push_back(depth);
#else
                total_depth += depth;
                valid_pixels += 1;
#endif
            }
        }

#if DILATE_MEDIAN
    std::sort(values.begin(), values.end());
    return (values.empty() ? 0.0f : values[values.size() / 2]);
#else
    return valid_pixels ? (total_depth / (float)valid_pixels) : 0.0f;
#endif
}

/* ---------------------------------------------------------------- */

FloatImage::Ptr
depthmap_fill (FloatImage::ConstPtr dm)
{
    FloatImage::Ptr ret(FloatImage::create(*dm));

    std::size_t ks = 5; // TODO Make input

    std::size_t w = dm->width();
    std::size_t h = dm->height();

    std::size_t i = 0;
    for (std::size_t y = 0; y < h; ++y)
        for (std::size_t x = 0; x < w; ++x, ++i)
            if (dm->at(i, 0) == 0.0f)
                ret->at(i, 0) = dilate_depth(dm, x, y, ks);

    return ret;
}

/* ---------------------------------------------------------------- */

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

    std::size_t w = ret->width();
    std::size_t h = ret->height();
    std::vector<bool> visited;
    visited.resize(w * h, false);

    std::size_t i = 0;
    for (std::size_t y = 0; y < h; ++y)
        for (std::size_t x = 0; x < w; ++x, ++i)
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
    if (!dm.get() || !cm.get())
        throw std::invalid_argument("NULL depth or confidence map");

    if (dm->width() != cm->width() || dm->height() != cm->height())
        throw std::invalid_argument("Image dimensions do not match");

    std::size_t cnt = dm->get_pixel_amount();
    for (std::size_t i = 0; i < cnt; ++i)
        if (cm->at(i, 0) <= 0.0f)
            dm->at(i, 0) = 0.0f;
}

/* ---------------------------------------------------------------- */

struct BilateralDepthCloseness
{
    float dc_sigma;

    BilateralDepthCloseness (float sigma) : dc_sigma(sigma) {}
    float operator() (math::Vec1f const& cv, math::Vec1f const& v)
    {
        return (v[0] <= 0.0f ? 0.0f
            : math::algo::gaussian(cv[0] - v[0], dc_sigma));
    }
};

FloatImage::Ptr
depthmap_bilateral_filter (FloatImage::ConstPtr dm,
    math::Matrix3f const& invproj, float gc_sigma, float pc_factor)
{
    if (!dm.get())
        throw std::invalid_argument("NULL image given");

    if (gc_sigma <= 0.0f || pc_factor <= 0.0f)
        throw std::invalid_argument("Invalid parameters given");

    /* Copy original depthmap. */
    FloatImage::Ptr ret(FloatImage::create(*dm));
    std::size_t w = dm->width();
    std::size_t h = dm->height();

    /* Calculate kernel size for geometric gaussian (see bilateral.h). */
    float ks_f = gc_sigma * 2.884f;
    std::size_t ks = std::ceil(ks_f);

    //std::cout << "Bilateral filtering depthmap with GC "
    //    << gc_sigma << " and PC factor " << pc_factor
    //    << ", kernel " << (ks*2+1) << "^2 pixels." << std::endl;

    /* Use standard geom closeness functors. */
    typedef BilateralGeomCloseness GCF;
    typedef BilateralDepthCloseness DCF;
    GCF gcf(gc_sigma);

    /* Apply kernel to each pixel. */
    std::size_t i = 0;
    for (std::size_t y = 0; y < h; ++y)
        for (std::size_t x = 0; x < w; ++x, ++i)
        {
            float depth = dm->at(i, 0);
            if (depth <= 0.0f)
                continue;

            float pixel_fp = mve::geom::pixel_footprint(x, y, depth, invproj);

            DCF dcf(pixel_fp * pc_factor);
            math::Vec1f v = bilateral_kernel<float, 1, GCF, DCF>
                (*dm, x, y, ks, gcf, dcf);
            ret->at(i, 0) = v[0];
        }

    return ret;
}

MVE_IMAGE_NAMESPACE_END
MVE_NAMESPACE_END

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

MVE_NAMESPACE_BEGIN
MVE_GEOM_NAMESPACE_BEGIN

#if 0

/* Old code from the ancient days where the K matrix was simple. */

float
pixel_footprint (std::size_t x, std::size_t y, float depth,
    std::size_t w, std::size_t h, float focal_len)
{
    float D = (float)std::max(w, h);
    float x2d = (float)x + 0.5f - 0.5f * (float)w;
    float y2d = (float)y + 0.5f - 0.5f * (float)h;
    float fl = focal_len * D;
    float fxy = std::sqrt(x2d * x2d + y2d * y2d + fl * fl);

    return depth / fxy;
}

math::Vec3f
pixel_3dpos (std::size_t x, std::size_t y, float depth,
    std::size_t w, std::size_t h, float focal_len)
{
/*
    math::Vec3f p3d((float)x + 0.5f - 0.5f * (float)w,
        (float)y + 0.5f - 0.5f * (float)h,
        focal_len * (float)std::max(w, h));
*/
    std::size_t xinv = w - x - 1;
    math::Vec3f p3d((float)xinv - 0.5f * (float)(w - 1),
        (float)y - 0.5f * (float)(h - 1),
        -focal_len * (float)std::max(w, h));

    return p3d.normalized() * depth;
}

#endif

/* ---------------------------------------------------------------- */

float
pixel_footprint (std::size_t x, std::size_t y, float depth,
    math::Matrix3f const& invproj)
{
    math::Vec3f v = invproj * math::Vec3f
        ((float)x + 0.5f, (float)y + 0.5f, 1.0f);
    float fp = invproj[0] * depth / v.norm();
    return -fp;
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
    std::size_t w(vidx.width());
    //std::size_t h(vidx.height());
    mve::TriangleMesh::VertexList& verts(mesh->get_vertices());
    mve::TriangleMesh::FaceList& faces(mesh->get_faces());

    for (int j = 0; j < 3; ++j)
    {
        std::size_t iidx = i + (tverts[j] % 2) + w * (tverts[j] / 2);
        std::size_t x = iidx % w;
        std::size_t y = iidx / w;
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
    if (!dm.get())
        throw std::invalid_argument("NULL depthmap given");

    std::size_t w = dm->width();
    std::size_t h = dm->height();

    /* Prepare triangle mesh. */
    TriangleMesh::Ptr mesh(TriangleMesh::create());

    /* Generate image that maps image pixels to vertex IDs. */
    mve::Image<unsigned int> vidx(w, h, 1);
    vidx.fill(MATH_MAX_UINT);

    /* Iterate over 2x2-blocks in the depthmap and create triangles. */
    std::size_t i = 0;
    for (std::size_t y = 0; y < h - 1; ++y, ++i)
    {
        //std::cout << "Row " << y << std::endl;
        for (std::size_t x = 0; x < w - 1; ++x, ++i)
        {
            /* Cache the four depth values. */
            float depths[4] = { dm->at(i, 0), dm->at(i + 1, 0),
                dm->at(i + w, 0), dm->at(i + w + 1, 0) };

            /* Create a mask representation of the available depth values. */
            int mask = 0;
            std::size_t pixels = 0;
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
    if (vids != 0)
        std::swap(vidx, *vids);

    return mesh;
}

/* ---------------------------------------------------------------- */
TriangleMesh::Ptr
depthmap_triangulate (FloatImage::ConstPtr dm, ByteImage::ConstPtr ci,
    math::Matrix3f const& invproj, float dd_factor)
{
    if (!dm.get())
        throw std::invalid_argument("NULL depthmap given");

    std::size_t w = dm->width();
    std::size_t h = dm->height();

    if (ci.get() && (ci->width() != w || ci->height() != h))
        throw std::invalid_argument("Color image dimension mismatch");

    /* Triangulate depth map. */
    mve::Image<unsigned int> vids;
    mve::TriangleMesh::Ptr mesh;
    mesh = mve::geom::depthmap_triangulate(dm, invproj, dd_factor, &vids);

    if (ci.get() == 0)
        return mesh;

    /* Use vertex index mapping to color the mesh. */
    mve::TriangleMesh::ColorList& colors(mesh->get_vertex_colors());
    mve::TriangleMesh::VertexList const& verts(mesh->get_vertices());
    colors.resize(verts.size());

    std::size_t num_pixel = vids.get_pixel_amount();
    for (std::size_t i = 0; i < num_pixel; ++i)
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
    if (!dm.get())
        throw std::invalid_argument("NULL depthmap given");
    if (cam.flen == 0.0f)
        throw std::invalid_argument("Invalid camera given");

    /* Triangulate depth map. */
    math::Matrix3f invproj;
    cam.fill_inverse_projection(*invproj, dm->width(), dm->height());
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
#if 0
    /* Depth discontinuity detection based on edge length ratio. */
    float len[3] = { (v2 - v1).square_norm(),
        (v3 - v2).square_norm(), (v1 - v3).square_norm() };
    float min = math::algo::min(len[0], len[1], len[2]);
    float max = math::algo::max(len[0], len[1], len[2]);
    return (std::sqrt(min) / std::sqrt(max)) < 0.2f;
#endif

#if 1
#   define ANGLE_THRES ((float)MATH_DEG2RAD(15.0f))
    /* Depth discontinuity detection based on minimal angle in triangle. */
    math::Vec3f e[3] = { (v2 - v1).normalized(),
        (v3 - v2).normalized(), (v1 - v3).normalized() };
    float min_angle = ANGLE_THRES;
    for (int i = 0; i < 3; ++i)
        min_angle = std::min(min_angle, std::acos(e[i].dot(-e[(i + 1) % 3])));
    //std::cout << "Min angle is " << MATH_RAD2DEG(min_angle) << ", "
    //    << (min_angle < ANGLE_THRES) << std::endl;
    return min_angle < ANGLE_THRES;
#endif
}

void
rangegrid_triangulate (Image<unsigned int> const& grid, TriangleMesh::Ptr mesh)
{
    if (!mesh.get())
        throw std::invalid_argument("NULL mesh given");

    std::size_t w(grid.width());
    std::size_t h(grid.height());

    TriangleMesh::VertexList const& verts(mesh->get_vertices());
    TriangleMesh::FaceList& faces(mesh->get_faces());

    for (std::size_t y = 0; y < h - 1; ++y)
        for (std::size_t x = 0; x < w - 1; ++x)
        {
            std::size_t i = y * w + x;

            unsigned int vid[4] = { grid[i], grid[i + 1],
                grid[i + w], grid[i + w + 1] };

            /* Create a mask representation of the available indices. */
            int mask = 0;
            std::size_t indices = 0;
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
    if (!mesh.get())
        throw std::invalid_argument("NULL mesh given");

    if (iterations < 0)
        throw std::invalid_argument("Invalid amount of iterations");

    if (iterations == 0)
        return;

    TriangleMesh::ConfidenceList& c(mesh->get_vertex_confidences());
    TriangleMesh::VertexList const& verts(mesh->get_vertices());
    c.clear();
    c.resize(verts.size(), 1.0f);

    /* Find boundary vertices and remember them. */
    typedef std::set<std::size_t> VIndexMap;
    VIndexMap vidx;
    VertexInfoList::Ptr vinfo(VertexInfoList::create(mesh));

    for (std::size_t i = 0; i < vinfo->size(); ++i)
    {
        MeshVertexInfo const& info(vinfo->at(i));
        if (info.vclass == VERTEX_CLASS_BORDER)
            vidx.insert(i);
    }

    /* Iteratively expand the current region and update confidences. */
    for (int current = 0; current < iterations; ++current)
    {
        /* Calculate confidence for that iteration. */
        float conf = (float)current / (float)iterations;
        conf = math::algo::fastpow(conf, 3);

        /* Assign current confidence to all vertices. */
        for (VIndexMap::iterator i = vidx.begin(); i != vidx.end(); ++i)
            c[*i] = conf;

        /* Replace vertex list with adjacent vertices. */
        VIndexMap cvidx;
        std::swap(vidx, cvidx);
        for (VIndexMap::iterator i = cvidx.begin(); i != cvidx.end(); ++i)
        {
            MeshVertexInfo info = vinfo->at(*i);
            for (std::size_t j = 0; j < info.verts.size(); ++j)
                if (c[info.verts[j]] == 1.0f)
                    vidx.insert(info.verts[j]);
        }
    }
}

/* ---------------------------------------------------------------- */

void
depthmap_mesh_peeling (TriangleMesh::Ptr mesh, int iterations)
{
    if (!mesh.get())
        throw std::invalid_argument("NULL mesh given");

    if (iterations < 0)
        throw std::invalid_argument("Invalid amount of iterations");

    if (iterations == 0)
        return;

    TriangleMesh::FaceList& faces(mesh->get_faces());
    std::vector<bool> dlist;
    dlist.resize(faces.size(), false);

    /* Iteratively invalidate triangles at the boundary. */
    for (int iter = 0; iter < iterations; ++iter)
    {
        VertexInfoList::Ptr vinfo(VertexInfoList::create(mesh));
        for (std::size_t i = 0; i < vinfo->size(); ++i)
        {
            MeshVertexInfo const& info(vinfo->at(i));
            if (info.vclass == VERTEX_CLASS_BORDER)
                for (std::size_t j = 0; j < info.faces.size(); ++j)
                    for (int k = 0; k < 3; ++k)
                    {
                        std::size_t fidx(info.faces[j] * 3 + k);
                        faces[fidx] = 0;
                        dlist[fidx] = true;
                    }
        }
    }

    /* Remove invalidated faces. */
    math::algo::vector_clean(faces, dlist);
}

MVE_GEOM_NAMESPACE_END
MVE_NAMESPACE_END
