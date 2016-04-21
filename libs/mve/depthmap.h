/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef MVE_DEPTHMAP_HEADER
#define MVE_DEPTHMAP_HEADER

#include "math/vector.h"
#include "math/matrix.h"
#include "mve/defines.h"
#include "mve/camera.h"
#include "mve/image.h"
#include "mve/mesh.h"

MVE_NAMESPACE_BEGIN
MVE_IMAGE_NAMESPACE_BEGIN

/**
 * Algorithm to clean small confident islands in the depth maps.
 * Islands that are smaller than 'thres' pixels are removed.
 * Zero depth values are considered unreconstructed.
 */
FloatImage::Ptr
depthmap_cleanup (FloatImage::ConstPtr dm, std::size_t thres);

/**
 * Removes the backplane according to the confidence map IN-PLACE.
 * Depth map values are reset to zero where confidence is leq 0.
 */
void
depthmap_confidence_clean (FloatImage::Ptr dm, FloatImage::ConstPtr cm);

/**
 * Filters the given depthmap using a bilateral filter.
 *
 * The filter smoothes similar depth values but preserves depth
 * discontinuities using gaussian weights for both, geometric
 * closeness in image space and geometric closeness in world space.
 *
 * Geometric closeness in image space is controlled by 'gc_sigma'
 * (useful values in [1, 20]). Photometric closeness is evaluated by
 * calculating the pixel footprint multiplied with 'pc_factor' to
 * detect depth discontinuities (useful values in [1, 20]).
 */
FloatImage::Ptr
depthmap_bilateral_filter (FloatImage::ConstPtr dm,
    math::Matrix3f const& invproj, float gc_sigma, float pc_fator);

/**
 * Converts between depth map conventions IN-PLACE. In one convention,
 * a depth map with a constant value means a plane, in another convention,
 * which is what MVE uses, a constant value creates a curved surface. The
 * difference is whether only the z-value is considered, or the distance
 * to the camera center is used (MVE).
 */
template <typename T>
void
depthmap_convert_conventions (typename Image<T>::Ptr dm,
    math::Matrix3f const& invproj, bool to_mve);

MVE_IMAGE_NAMESPACE_END
MVE_NAMESPACE_END

/* ---------------------------------------------------------------- */

MVE_NAMESPACE_BEGIN
MVE_GEOM_NAMESPACE_BEGIN

/**
 * Function that calculates the pixel footprint (pixel width)
 * in 3D coordinates for pixel (x,y) and 'depth' for a depth map
 * with inverse K matrix 'invproj'.
 */
float
pixel_footprint (std::size_t x, std::size_t y, float depth,
    math::Matrix3f const& invproj);

/**
 * Function that calculates the pixel 3D position in camera coordinates for
 * pixel (x,y) and 'depth' for a depth map with inverse K matrix 'invproj'.
 */
math::Vec3f
pixel_3dpos (std::size_t x, std::size_t y, float depth,
    math::Matrix3f const& invproj);

/**
 * Algorithm to triangulate depth maps.
 *
 * A factor may be specified that guides depth discontinuity detection. A
 * depth discontinuity between pixels is assumed if depth difference is
 * larger than pixel footprint times 'dd_factor'. If 'dd_factor' is zero,
 * no depth discontinuity detection is performed. The depthmap is
 * triangulated in the local camera coordinate system.
 *
 * If 'vids' is not null, image content is replaced with vertex indices for
 * each pixel that generated the vertex. Index MATH_MAX_UINT corresponds to
 * a pixel that did not generate a vertex.
 */
TriangleMesh::Ptr
depthmap_triangulate (FloatImage::ConstPtr dm, math::Matrix3f const& invproj,
    float dd_factor = 5.0f, mve::Image<unsigned int>* vids = nullptr);

/**
 * A helper function that triangulates the given depth map with optional
 * color image (which generates additional per-vertex colors) in local
 * image coordinates.
 */
TriangleMesh::Ptr
depthmap_triangulate (FloatImage::ConstPtr dm, ByteImage::ConstPtr ci,
	math::Matrix3f const& invproj, IntImage::ConstPtr vi = nullptr, float dd_factor = 5.0f);

/**
 * A helper function that triangulates the given depth map with optional
 * color image (which generates additional per-vertex colors) and transforms
 * the mesh into the global coordinate system.
 */
TriangleMesh::Ptr
depthmap_triangulate (FloatImage::ConstPtr dm, ByteImage::ConstPtr ci,
	CameraInfo const& cam, IntImage::ConstPtr vi = nullptr, float dd_factor = 5.0f);

/**
 * Algorithm to triangulate range grids.
 * Vertex positions are given in 'mesh' and a grid that contains vertex
 * indices is specified. Four indices are taken at a time and triangulated
 * with discontinuity detection. New triangles are added to the mesh.
 */
void
rangegrid_triangulate (Image<unsigned int> const& grid, TriangleMesh::Ptr mesh);

/**
 * Algorithm to assign per-vertex confidence values to vertices of
 * a triangulated depth map. Confidences are low near boundaries
 * and small regions.
 */
void
depthmap_mesh_confidences (TriangleMesh::Ptr mesh, int iterations = 3);

/**
 * Algorithm that peels away triangles at the mesh bounary of a
 * triangulated depth map. The algorithm also works on other mesh
 * data but is particularly useful for MVS depthmap where the edges
 * of the real object are extended beyond their correct position.
 */
void
depthmap_mesh_peeling (TriangleMesh::Ptr mesh, int iterations = 1);

MVE_GEOM_NAMESPACE_END
MVE_NAMESPACE_END

/* ------------------------- Implementation ----------------------- */

MVE_NAMESPACE_BEGIN
MVE_IMAGE_NAMESPACE_BEGIN

template <typename T>
inline void
depthmap_convert_conventions (typename Image<T>::Ptr dm,
    math::Matrix3f const& invproj, bool to_mve)
{
    std::size_t w = dm->width();
    std::size_t h = dm->height();
    std::size_t pos = 0;
    for (std::size_t y = 0; y < h; ++y)
        for (std::size_t x = 0; x < w; ++x, ++pos)
        {
            // Construct viewing ray for that pixel
            math::Vec3f px((float)x + 0.5f, (float)y + 0.5f, 1.0f);
            px = invproj * px;
            // Measure length of viewing ray
            double len = px.norm();
            // Either divide or multiply with the length
            dm->at(pos) *= (to_mve ? len : 1.0 / len);
        }
}

MVE_IMAGE_NAMESPACE_END
MVE_NAMESPACE_END

#endif /* MVE_DEPTHMAP_HEADER */
