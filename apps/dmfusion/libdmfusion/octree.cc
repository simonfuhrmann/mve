#include <cerrno> // for errno
#include <cstring> // for ::strerror

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <limits>
#include <stack>
#include <set>
#include <map>

#include "util/exception.h"
#include "math/matrix.h"
#include "math/vector.h"
#include "math/octree_tools.h"
#include "mve/mesh_tools.h"
#include "mve/depthmap.h"

#include "octree.h"

DMFUSION_NAMESPACE_BEGIN

VoxelIndex
VoxelIndex::descend (void) const
{
    math::Vec3st idx;
    this->factor_index(this->index, *idx);
    idx = idx * 2;

    VoxelIndex ret(this->level + 1, 0);
    ret.set_index(*idx);
    return ret;
}

VoxelIndex
VoxelIndex::navigate (int x, int y, int z) const
{
    std::size_t idx[3];
    this->factor_index(this->index, idx);
    idx[0] = (x < 0 && (std::size_t)-x > idx[0]) ? 0 : idx[0] + x;
    idx[1] = (y < 0 && (std::size_t)-y > idx[1]) ? 0 : idx[1] + y;
    idx[2] = (z < 0 && (std::size_t)-z > idx[2]) ? 0 : idx[2] + z;
    VoxelIndex ret(this->level, 0);
    ret.set_index(idx);
    return ret;
}

bool
VoxelIndex::is_neighbor (VoxelIndex const& other) const
{
    if (this->level < other.level)
        return other.is_neighbor(*this);

    std::size_t oi[3];
    other.factor_index(other.index, oi);
    std::size_t ti[3];
    this->factor_index(this->index, ti);

    int ld = this->level - other.level;
    //std::size_t off = (ld ? (1 << ld) : 2);
    std::size_t off = (1 << ld) + 2; // TODO +2? +1?
    for (int i = 0; i < 3; ++i)
    {
        oi[i] = oi[i] << ld;
        if (ti[i] + off < oi[i] || oi[i] + off < ti[i])
            return false;
    }

    return true;
}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void
Octree::insert (mve::FloatImage::ConstPtr dm, mve::ByteImage::ConstPtr ci,
    mve::CameraInfo const& cam, int conf_iter)
{
    //std::cout << "Converting depthmap to mesh..." << std::endl;

    mve::TriangleMesh::Ptr mesh = mve::geom::depthmap_triangulate(dm, ci, cam);

    /* Apply confidences to mesh vertices. */
    mve::geom::depthmap_mesh_confidences(mesh, conf_iter);

    /* Insert depth map as regular mesh. */
    math::Vec3f campos;
    cam.fill_camera_pos(*campos);
    this->insert(mesh, campos);
}

/* ---------------------------------------------------------------- */

void
Octree::insert (mve::TriangleMesh::ConstPtr mesh, math::Vec3f const& campos)
{
    if (!mesh.get())
        throw std::invalid_argument("NULL mesh given");

    mve::TriangleMesh::VertexList const& verts(mesh->get_vertices());
    mve::TriangleMesh::FaceList const& faces(mesh->get_faces());
    mve::TriangleMesh::NormalList const& normals(mesh->get_vertex_normals());
    mve::TriangleMesh::ColorList const& colors(mesh->get_vertex_colors());
    mve::TriangleMesh::ConfidenceList const& confs(mesh->get_vertex_confidences());

    if (faces.empty() || verts.empty())
    {
        std::cout << "Warning: Skipping mesh without faces!" << std::endl;
        return;
    }

    if (normals.size() != verts.size())
        throw std::invalid_argument("Mesh without vertex normals given");

    //std::cout << "Inserting mesh into octree..." << std::endl;

    /* Calculate AABB of the mesh. */
    math::Vec3f aabb_min, aabb_max;
    mve::geom::mesh_find_aabb(mesh, aabb_min, aabb_max);

    /* Enlarge AABB to create a safety border around the mesh. */
    math::Vec3f border_hs((aabb_max - aabb_min));
    border_hs *= this->safety_border;
    aabb_min -= border_hs;
    aabb_max += border_hs;

    /* Create root if neccessary, or expand it. */
    if (this->voxels.empty() && !this->forced_aabb)
    {
        /* No root. Create it. */
        this->create_root(aabb_min, aabb_max);
    }
    else if (this->allow_expansion && !this->forced_aabb)
    {
        /* Check if AABB fits in octree node. */
        this->expand_root(aabb_min, aabb_max);
    }

    /* Log history to display amount of voxels per levels. */
    int levelhist[21];
    for (int i = 0; i < 21; ++i)
        levelhist[i] = 0;

    /* Provide all triangles to the octree. */
#pragma omp parallel for
    for (std::size_t i = 0; i < faces.size(); i += 3)
    {
        /* Prepare triangle information. */
        OctreeTriangle tri;
        for (int j = 0; j < 3; ++j)
        {
            tri.v[j] = verts[faces[i + j]];
            tri.n[j] = normals[faces[i + j]];
        }

        tri.has_colors = (colors.size() == verts.size());
        if (tri.has_colors)
            for (int j = 0; j < 3; ++j)
            {
                tri.c[j] = colors[faces[i + j]];
            }

        tri.has_confidences = (confs.size() == verts.size());
        if (tri.has_confidences)
            for (int j = 0; j < 3; ++j)
                tri.conf[j] = confs[faces[i + j]];

        if (!(i % 1000))
        {
            std::cout << "\rInserting triangles ("
                << (int)((float)i / (float)(faces.size() - 1) * 100.0f)
                << "%)..." << std::flush;
        }

        /* Insert, i.e. update SDF. */
        int l = this->insert(tri, campos);
        levelhist[l] += 1;
    }
    std::cout << " done." << std::endl;

    /* Print level history. */
    for (int i = 0; i < 20; ++i)
    {
        if (levelhist[i] == 0)
            continue;
        std::cout << "  Level " << std::setw(2) << i
            << ": " << std::setw(6) << levelhist[i] << std::endl;
    }
}

/* ---------------------------------------------------------------- */

int
Octree::insert (OctreeTriangle const& tri, math::Vec3f const& campos)
{
    //std::cout << "Inserting triangle..." << std::endl;

    /* Quickly handle the case with a fixed level. */
    if (this->forced_level)
    {
        /* Insert triangle into the forced level only. */
        this->insert(tri, this->forced_level, 1.0f, campos);
        return this->forced_level;
    }

    /*
     * Calculate triangle footprint and select level.
     */

    float len[3] =
    {
        (tri.v[0] - tri.v[1]).square_norm(),
        (tri.v[1] - tri.v[2]).square_norm(),
        (tri.v[2] - tri.v[0]).square_norm()
    };

    /* Find footprint (FP) by finding minimal edge length. */
    float tri_fp = std::sqrt(math::algo::min(len[0], len[1], len[2]));
    float root_fp = this->halfsize * 2.0f;

    /* Calculate optimal destination level. */
    float log2 = std::log(root_fp / tri_fp * this->sampling_rate) / MATH_LN2;
    int level = static_cast<int>(std::ceil(log2));

    /* Insert in optimal level and in a few coarser levels. */
    int target_level = std::max(0, level - this->coarser_levels);
    for (int i = level; i >= target_level; --i)
        this->insert(tri, i, 1.0f, campos);

    return level;
}

/* ---------------------------------------------------------------- */

void
Octree::insert (OctreeTriangle const& tri, int level,
    float level_weight, math::Vec3f const& campos)
{
    /*
     * Triangle 'tri' is to be inserted at 'level'.
     * 1. Calculate rampsize for the given level.
     * 2. Create truncated tetrahedron from triangle and rampsize.
     * 3. Create AABB that contains truncated tetrahedron.
     * 4. Identify all voxels at 'level' in AABB.
     * 5. Calculate distance from 'tri' to identified voxels.
     */

    /* 1. Calculate ramp size. */
    float level_fp = this->halfsize * 2.0f;
    level_fp /= (float)(1 << level);
    float ramp_len = this->ramp_factor * level_fp;

    /* 2. + 3. Calculate AABB for truncated tetrahedron. */
    math::Vec3f aabb_min(std::numeric_limits<float>::max());
    math::Vec3f aabb_max(-std::numeric_limits<float>::max());
    for (int i = 0; i < 3; ++i)
    {
        math::Vec3f d = (tri.v[i] - campos).normalized();
        math::Vec3f p[2] = { tri.v[i] + d * ramp_len, tri.v[i] - d * ramp_len };
        for (int k = 0; k < 2; ++k)
            for (int j = 0; j < 3; ++j)
            {
                if (p[k][j] < aabb_min[j])
                    aabb_min[j] = p[k][j];
                if (p[k][j] > aabb_max[j])
                    aabb_max[j] = p[k][j];
            }
    }

    /* Identify all voxels in AABB. */
    typedef std::vector<VoxelIndex> VoxelVector;
    VoxelVector sdf_voxels;

    {
        math::Vec3f root_aabb_min(this->aabb_min());
        std::size_t dim(1 << level);
        std::size_t min_id[3], max_id[3];
        float hs2 = this->halfsize * 2.0f;
        float fdim = (float)dim;

        for (int i = 0; i < 3; ++i)
        {
            float fmin = (aabb_min[i] - root_aabb_min[i]) * fdim / hs2 - 0.1f;
            float fmax = (aabb_max[i] - root_aabb_min[i]) * fdim / hs2 + 0.1f;
            fmin = math::algo::clamp(fmin, 0.0f, fdim);
            fmax = math::algo::clamp(fmax, 0.0f, fdim);
            min_id[i] = (std::size_t)std::ceil(fmin);
            max_id[i] = (std::size_t)std::floor(fmax);
            if (fmin == fmax)
                return; // Triange outside AABB
        }

        std::size_t xyz[3];
        for (xyz[2] = min_id[2]; xyz[2] <= max_id[2]; ++xyz[2])
            for (xyz[1] = min_id[1]; xyz[1] <= max_id[1]; ++xyz[1])
                for (xyz[0] = min_id[0]; xyz[0] <= max_id[0]; ++xyz[0])
                {
                    VoxelIndex vi(level, 0);
                    vi.set_index(xyz);

                    /* TODO: Calc position and make in-tet check. */

                    sdf_voxels.push_back(vi);
                }
    }

    /* Step 5: Calculate distance from voxels to triangle. */
    for (std::size_t i = 0; i < sdf_voxels.size(); ++i)
    {
        VoxelIndex vi(sdf_voxels[i]);

        /* Calculate ray from camera center through voxel. */
        math::Vec3f vpos = vi.get_pos(this->center, this->halfsize);
        float camdist = (vpos - campos).norm();
        math::Vec3f ray = (vpos - campos) / camdist;
        //math::Vec3f ray = this->use_orthographic
        //    ? this->viewdir
        //    : (vpos - campos) / camdist;

        /* Calc signed distance from cam center through voxel to surface. */
        math::Vec3f bary;
        float dist = math::geom::ray_triangle_intersect<float>
                (campos, ray, tri.v[0], tri.v[1], tri.v[2], *bary);
        bary[2] = 1.0f - bary[0] - bary[1];

        /* Skip voxel if ray did not hit the triangle (dist == 0). */
        /* Skip voxel if located behind the camera center (dist < 0). */
        /* FIXME: Don't use zero to indicate an invalid distance! */
        if (dist == 0.0f || dist < 0.0f)
            continue;

        /* Subtract distance from camera to voxel to get SDF value. */
        dist -= camdist;

        /* Skip voxel if it is located beyond ramp extends. */
        if (std::abs(dist) > ramp_len)
            continue;

        /* Calculate normal on hitpoint. */
        math::Vec3f normal(tri.n[0] * bary[0] + tri.n[1] * bary[1]
            + tri.n[2] * bary[2]);
        normal.normalize();

        /* Calculate angle weight, the angle between the ray and triangle. */
        float angle_weight = -normal.dot(ray);
        if (angle_weight < 0.0f)
        {
            /* The ray can hit a backface for two reasons:
             * 1. The voxel is behind the camera center, thus creating a
             *    reverse ray that evaluates to hitting a backface.
             * 2. The ray really hits a backface because the camera center
             *    and/or projection assumptions are wrong.
             * Currently there is no way to distinguish between these cases.
             */
            std::cout << "Warning: Ray hit triangle backface!" << std::endl;
#if 0
            std::cout << "Level " << level << ", Bary: " << bary << std::endl;
            std::cout << "Triangle: " << tri.n[0] << " / " << tri.n[1] << " / " << tri.n[2] << std::endl;
            std::cout << "Interpolated normal: " << normal << std::endl;
            std::cout << "Ray: " << ray << std::endl;

            std::ofstream testout("/tmp/testout.off");
            testout << "OFF" << std::endl << "5 1 0" << std::endl;
            testout << tri.v[0] << std::endl << tri.v[1] << std::endl << tri.v[2] << std::endl;
            testout << campos << std::endl << vpos << std::endl;
            testout << "3 0 1 2" << std::endl;
            testout.close();
#endif
            continue;
        }

        /* Calculate distance weight, weight falloff with distance. */
        // http://www.graphics.stanford.edu/software/vrip/guide/
        float dist_weight = math::algo::clamp
            (2.0f * (1.0f - std::abs(dist) / ramp_len),
            0.0f, 1.0f);

        /* Calculate confidence weight. */
        float conf_weight = !tri.has_confidences ? 1.0f
            : tri.conf[0] * bary[0] + tri.conf[1] * bary[1]
            + tri.conf[2] * bary[2];

        /* Calculate color for hitpoint. */
        math::Vec4f color(0.0f);
        if (tri.has_colors)
            color = tri.c[0] * bary[0]
                + tri.c[1] * bary[1] + tri.c[2] * bary[2];

        float weight = level_weight * angle_weight * dist_weight * conf_weight;
        if (weight <= 0.0f)
            continue;

        /* Get voxel (this creates if it does not exist), and assign values. */
#pragma omp critical
        {
        VoxelData& voxel = this->voxels[vi];
        if (voxel.weight == 0.0f)
        {
            voxel.weight = weight;
            voxel.dist = dist;
            voxel.color = color;
            voxel.color[3] = weight;
        }
        else
        {
            float total_weight = voxel.weight + weight;
            float w1 = voxel.weight / total_weight;
            float w2 = weight / total_weight;
            voxel.dist = voxel.dist * w1 + dist * w2;
            voxel.weight = total_weight;
            if (tri.has_colors)
            {
                float total_cweight = voxel.color[3] + weight;
                float cw1 = voxel.color[3] / total_cweight;
                float cw2 = weight / total_cweight;
                voxel.color = voxel.color * cw1 + color * cw2;
                voxel.color[3] = total_cweight;
            }
        }
        }
    }
}

/* ---------------------------------------------------------------- */

void
Octree::create_root (math::Vec3f const& min, math::Vec3f const& max)
{
    std::cout << "Note: Creating root..." << std::endl;
    this->center = (min + max) * 0.5f;
    this->halfsize = (max - min).maximum() * 0.5f;
}

/* ---------------------------------------------------------------- */

void
Octree::expand_root (math::Vec3f const& min, math::Vec3f const& max)
{
    /* Check if new AABB fits into octree. */
    bool fits = true;
    math::Vec3f rmin(this->aabb_min());
    math::Vec3f rmax(this->aabb_max());
    for (int i = 0; i < 3 && fits; ++i)
    {
        if (min[i] < rmin[i] || max[i] > rmax[i])
            fits = false;
    }

    if (fits)
        return;

    std::cout << "Note: Expanding octree root!" << std::endl;

    /* Determine new octant of current node. */
    signed char octant = 0;
    math::Vec3f const& c(this->center);
    for (int i = 0; i < 3; ++i)
        if (std::abs(c[i] - min[i]) > std::abs(c[i] - max[i]))
            octant |= (1 << i);

    //std::cout << "Octant: " << (int)octant << std::endl;

    /* Create new octree root and replace current root. */
    float old_halfsize = this->halfsize;
    this->halfsize *= 2.0f;

    for (int i = 0; i < 3; ++i)
        if (octant & (1 << i))
            this->center[i] -= old_halfsize;
        else
            this->center[i] += old_halfsize;

    //this->cur_level += 1;

    /*
     * Updating the root node requires to update all active voxel
     * indices in the VoxelSet. Yes, that's annoying.
     * Depending on the octant, the old indices are offset
     * to adjust the indices to the new octree root.
     */
    VoxelMap old_voxels;
    std::swap(this->voxels, old_voxels);
    for (VoxelMap::iterator i = old_voxels.begin();
        i != old_voxels.end(); ++i)
    {
        VoxelIndex vi(i->first);
        std::size_t idx[3];
        vi.factor_index(vi.index, idx);
        std::size_t one(1);
        std::size_t off = one << vi.level;
        for (int j = 0; j < 3; ++j)
            if (octant & (1 << j))
                idx[j] += off;

        std::size_t dim = (one << (vi.level + 1)) + one;
        std::size_t new_index = idx[0] + idx[1] * dim + idx[2] * dim * dim;
        VoxelIndex new_vi(vi.level + 1, new_index);
        this->voxels.insert(std::make_pair(new_vi, i->second));
    }

    /* Recursively expand until min/max fits. */
    this->expand_root(min, max);
}

/* ---------------------------------------------------------------- */

VoxelData const*
Octree::find_voxel (VoxelIndex const& vi) const
{
    VoxelMap::const_iterator iter = this->voxels.find(vi);
    if (iter == this->voxels.end())
        return 0;
    return &iter->second;
}

/* ---------------------------------------------------------------- */

VoxelData*
Octree::find_voxel (VoxelIndex const& vi)
{
    VoxelMap::iterator iter = this->voxels.find(vi);
    if (iter == this->voxels.end())
        return 0;
    return &iter->second;
}

/* ---------------------------------------------------------------- */

bool
Octree::erase_voxel (VoxelIndex const& index)
{
    return this->voxels.erase(index);
}

/* ---------------------------------------------------------------- */

std::size_t
Octree::get_memory_usage (void) const
{
    std::size_t size = voxels.size() * (sizeof(VoxelMap::key_type)
        + sizeof(VoxelMap::mapped_type));
    std::cout << "Memory: " << (size >> 20) << "MB for "
        << voxels.size() << " voxels." << std::endl;
    return size;
}

/* ---------------------------------------------------------------- */

void
Octree::save_octree (std::string const& filename) const
{
    std::ofstream out(filename.c_str());
    if (!out.good())
        throw util::FileException(filename, ::strerror(errno));

    out << "DMFOCTREE" << std::endl;
    out << this->voxels.size() << std::endl;
    out.write((char const*)*this->center, sizeof(float) * 3);
    out.write((char const*)&this->halfsize, sizeof(float));

    for (VoxelMap::const_iterator i = this->voxels.begin();
        i != this->voxels.end(); ++i)
    {
        VoxelIndex const& vi(i->first);
        VoxelData const& vd(i->second);

        out.write((char const*)&vi.level, sizeof(char));
        out.write((char const*)&vi.index, sizeof(std::size_t));
        out.write((char const*)&vd.dist, sizeof(float));
        out.write((char const*)&vd.weight, sizeof(float));
        out.write((char const*)*vd.color, sizeof(float) * 4);
    }
    out.close();
}

/* ---------------------------------------------------------------- */

void
Octree::load_octree (std::string const& filename)
{
    if (!this->voxels.empty())
        this->clear();

    std::ifstream in(filename.c_str());
    if (!in.good())
        throw util::FileException(filename, ::strerror(errno));

    std::string buf;

    /* Read file header. */
    std::getline(in, buf);
    if (buf != "DMFOCTREE")
    {
        in.close();
        throw util::Exception("File format not recognized");
    }

    /* Read amount of voxels. */
    std::getline(in, buf);
    std::size_t num_voxels = util::string::convert<std::size_t>(buf);

    /* Read center and halfsize of root node. */
    float center_and_hs[4];
    in.read((char*)center_and_hs, sizeof(float) * 4);
    this->center = math::Vec3f(center_and_hs);
    this->halfsize = center_and_hs[3];

    std::size_t levelhist[21];
    std::fill(levelhist, levelhist + 21, 0);

    std::cout << "Octree contains " << num_voxels << " voxels." << std::endl;

    /* Read all voxels and insert. */
    //std::cout << "Reading " << num_voxels << " voxels..." << std::endl;
    for (std::size_t i = 0; i < num_voxels; ++i)
    {
        VoxelIndex vi;
        VoxelData vd;
        in.read((char*)&vi.level, sizeof(char));
        in.read((char*)&vi.index, sizeof(std::size_t));
        in.read((char*)&vd.dist, sizeof(float));
        in.read((char*)&vd.weight, sizeof(float));
        in.read((char*)*vd.color, sizeof(float) * 4);
        this->voxels.insert(std::make_pair(vi, vd));
        levelhist[(int)vi.level] += 1;

        if (!(i % 100000))
            std::cout << "\rLoading octree from file ("
                << (int)((float)i / (float)num_voxels * 100.0f)
                << "%)..." << std::flush;
    }
    std::cout << " done." << std::endl;
    in.close();

    /* Print level history. */
    for (int i = 0; i < 21; ++i)
        if (levelhist[i])
            std::cout << "  Level " << i << ": " << levelhist[i] << std::endl;
}

/* ---------------------------------------------------------------- */

mve::FloatImage::Ptr
Octree::get_slice (int level, int axis, std::size_t id)
{
    std::size_t one(1);
    std::size_t dim = (one << level) + one;

    if (id >= dim)
        throw std::invalid_argument("ID out of bounds");
    if (axis < 0 || axis > 2)
        throw std::invalid_argument("Invalid axis ID");

    /* Assign axis offset depending on selected axis. */
    std::size_t dimx(1), dimy(dim), dimz(dim * dim);
    if (axis == 0)
    {
        dimx = dim * dim;
        dimy = dim;
        dimz = one;
    }
    else if (axis == 1)
    {
        dimx = one;
        dimy = dim * dim;
        dimz = dim;
    }

    /* Create output image. */
    mve::FloatImage::Ptr ret(mve::FloatImage::create(dim, dim, 6));
    ret->fill(0.0f);

#if 1
    /* Faster algorithm that iterates over all voxel of the level */
    VoxelIndex vi_start(level, 0);
    VoxelIndex vi_end(level, std::numeric_limits<std::size_t>::max());
    VoxelMap::iterator start = this->voxels.lower_bound(vi_start);
    VoxelMap::iterator end = this->voxels.upper_bound(vi_end);

    for (VoxelMap::iterator i = start; i != end && i != this->voxels.end(); ++i)
    {
        VoxelIndex const& vi(i->first);
        VoxelData const& vd(i->second);
        if ((vi.index / dimz) % dim != id)
            continue;
        std::size_t x = (vi.index / dimx) % dim;
        std::size_t y = (vi.index / dimy) % dim;
        std::size_t index = x + y * dim;
        ret->at(index, 0) = vd.dist;
        ret->at(index, 1) = vd.weight;
                for (int i = 0; i < 4; ++i)
                    ret->at(index, i + 2) = vd.color[i];
    }
#else
    /* Slower algorithm that tries to lookup all indices */
    std::size_t index = 0;
    for (std::size_t y = 0; y < dim; ++y)
        for (std::size_t x = 0; x < dim; ++x, ++index)
        {
            std::size_t voxel_index = x * dimx + y * dimy + id * dimz;
            VoxelIndex vi(level, voxel_index);
            VoxelData const* voxel = this->find_voxel(vi);

            if (voxel)
            {
                ret->at(index, 0) = voxel->dist;
                ret->at(index, 1) = voxel->weight;
                for (int i = 0; i < 4; ++i)
                    ret->at(index, i + 2) = voxel->color[i];
            }
        }
#endif

    return ret;
}

/* ---------------------------------------------------------------- */

void
Octree::boost_voxels (float thres)
{
    if (this->voxels.empty() || thres <= 0.0f)
        return;

    thres = std::max(0.0f, thres);
    VoxelMap vmap;

    std::size_t cont_stats[3] = { 0, 0, 0 };

    for (VoxelMap::iterator iter = this->voxels.begin();
        iter != this->voxels.end(); ++iter)
    {
        VoxelIndex const& vi(iter->first);
        VoxelData vd(iter->second);

        /* Accept all voxels above threshold. */
        if (vd.weight >= thres)
        {
            vmap.insert(std::make_pair(vi, vd));
            cont_stats[0]++;
            continue;
        }

        /* For voxels below threshold, interpolate parent voxels. */
        std::size_t xyz[3];
        vi.factor_index(vi.index, xyz);
        std::size_t min_xyz[3];
        std::size_t max_xyz[3];
        for (int i = 0; i < 3; ++i)
            if (xyz[i] % 2)
            {
                min_xyz[i] = xyz[i] >> 1;
                max_xyz[i] = (xyz[i] >> 1) + 1;
            }
            else
            {
                min_xyz[i] = max_xyz[i] = xyz[i] >> 1;
            }

        // Should this also happen for unconfident parent voxels?
        float parent_weight = std::numeric_limits<float>::max();
        float parent_dist = 0.0f;
        int total_amount = 0;
        int expected_amount = 0;
        std::size_t ixyz[3];
        for (ixyz[2] = min_xyz[2]; ixyz[2] <= max_xyz[2]; ++ixyz[2])
            for (ixyz[1] = min_xyz[1]; ixyz[1] <= max_xyz[1]; ++ixyz[1])
                for (ixyz[0] = min_xyz[0]; ixyz[0] <= max_xyz[0]; ++ixyz[0])
                {
                    expected_amount += 1;
                    VoxelIndex pvi(vi.level - 1, 0);
                    pvi.set_index(ixyz);
                    VoxelMap::iterator i = this->voxels.find(pvi);
                    if (i == this->voxels.end())
                        continue;

                    float weight = std::min(thres, i->second.weight);
                    parent_dist += i->second.dist;
                    parent_weight = std::min(parent_weight, weight);
                    total_amount += 1;
                }

        if (total_amount != expected_amount)
        {
            vmap.insert(std::make_pair(vi, vd));
            cont_stats[1]++;
            continue;
        }

        if (total_amount == 0 || parent_weight <= 0.0f)
        {
            vmap.insert(std::make_pair(vi, vd));
            cont_stats[2]++;
            continue;
        }

        /* Normalize distance. */
        parent_dist /= (float)total_amount;

        /* Apply. */

        // Variant 1
        float pweight = parent_weight / thres * (thres - vd.weight);
        float nweight = (pweight + vd.weight);
        vd.dist = (pweight * parent_dist + vd.weight * vd.dist) / nweight;
        vd.weight = nweight;

        // Variant 2
        /*
        vd.dist = parent_weight * parent_dist + vd.weight * vd.dist;
        vd.weight = parent_weight + vd.weight;
        vd.dist /= vd.weight;
        */

        MATH_NAN_CHECK(vd.dist)
        MATH_NAN_CHECK(vd.weight)

        vmap.insert(std::make_pair(vi, vd));
    }

    //std::cout << "Stats: " << cont_stats[0] << " "
    //    << cont_stats[1] << " " << cont_stats[2] << std::endl;

    std::swap(this->voxels, vmap);
}

/* ---------------------------------------------------------------- */

std::size_t
Octree::remove_twins (void)
{
    VoxelMap new_vmap;
    std::size_t erased_redundant = 0;
    while (!this->voxels.empty())
    {
        VoxelMap::iterator i = this->voxels.begin();
        /* Collect all voxels at this position. */
        std::vector<VoxelMap::iterator> vil;
        vil.push_back(i);
        for (dmf::VoxelIndex vi = i->first.descend();
            vi.level < 21; vi = vi.descend())
        {
            VoxelMap::iterator desc = this->voxels.find(vi);
            if (desc != this->voxels.end())
                vil.push_back(desc);
        }

        /*
         * Strategies to eliminate duplicated vertices:
         * Remove all dups of the voxel but remember most confident.
         * Remove all dups and take average. (what level to choose?)
         * Remove all dups but remember voxel at deepest level.
         */

        /* Remember one representant for all the voxels that coincide. */
        VoxelMap::iterator& iter = vil.back();
        new_vmap.insert(std::make_pair(iter->first, iter->second));

        /* Erase all collected voxels in the octree map. */
        for (std::size_t j = 0; j < vil.size(); ++j)
            this->voxels.erase(vil[j]);
        erased_redundant += vil.size() - 1;
    }
    this->voxels.clear();
    std::swap(this->voxels, new_vmap);

    return erased_redundant;
}

/* ---------------------------------------------------------------- */

std::size_t
Octree::remove_unconfident (float thres)
{
    std::size_t erased_unconfident = 0;

    for (VoxelIter i = this->voxels.begin(); i != this->voxels.end(); )
    {
        dmf::VoxelData const& vd(i->second);
        if (vd.weight <= thres)
        {
            this->voxels.erase(i++);
            erased_unconfident += 1;
        }
        else
            ++i;
    }

    return erased_unconfident;
}

DMFUSION_NAMESPACE_END
