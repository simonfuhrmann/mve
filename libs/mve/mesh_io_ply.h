/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef MVE_PLY_FILE_HEADER
#define MVE_PLY_FILE_HEADER

#include <istream>
#include <string>

#include "util/endian.h"
#include "mve/defines.h"
#include "mve/image.h"
#include "mve/camera.h"
#include "mve/view.h"
#include "mve/mesh.h"

MVE_NAMESPACE_BEGIN
MVE_GEOM_NAMESPACE_BEGIN

/**
 * Loads a triangle mesh from a PLY model file.
 */
TriangleMesh::Ptr
load_ply_mesh (std::string const& filename);

/**
 * Loads a depth map from a PLY file.
 * Warning: Aspect ratio may get lost, depending on the range points.
 */
FloatImage::Ptr
load_ply_depthmap (std::string const& filename);

/**
 * Load XF file, typically with camera to world transformation.
 * This function simply reads the 16 float values into the
 * specified array.
 */
void
load_xf_file (std::string const& filename, float* ctw);

/**
 * Options struct for saving PLY files.
 */
struct SavePLYOptions
{
    bool format_binary = true;
    bool write_vertex_colors = true;
    bool write_vertex_normals = false;
    bool write_vertex_confidences = true;
    bool write_vertex_values = true;
	bool write_vertex_view_ids = false;
    bool write_face_colors = true;
    bool write_face_normals = false;
    unsigned int verts_per_simplex = 3;
	unsigned char view_ids_per_vertex = 4;
};

/**
 * Stores a PLY file from a triangle mesh. Colors are written
 * if 'write_colors' is true and colors are given. Vertex normals
 * are written if 'write_normals' is true and normals are given.
 */
void
save_ply_mesh (TriangleMesh::ConstPtr mesh, std::string const& filename,
    SavePLYOptions const& options = SavePLYOptions());

/**
 * Stores a scanalize-compatible PLY file from a depth map.
 * If the confidence map is given, confidence values are stored and
 * only those depth values with non-zero confidence values are stored.
 * The function optionally takes a color image to store color values.
 *
 * TODO: Replace camera with focal length? Only flen needed!
 */
void
save_ply_view (std::string const& filename, CameraInfo const& camera,
    FloatImage::ConstPtr depth_map,
    FloatImage::ConstPtr confidence_map = FloatImage::ConstPtr(nullptr),
    ByteImage::ConstPtr color_image = ByteImage::ConstPtr(nullptr));

/**
 * Stores a scanalize-compatible PLY file from a view. It selects
 * embeddings "depthmap", "confidence" and "undistorted".
 */
void
save_ply_view (View::Ptr view, std::string const& filename);

/**
 * Stores a scanalyze-compatible PLY file from a view by specifying the
 * names of the embeddings for depthmap, confidence map and color image.
 * Names for 'confidence' and 'color_image' may be empty.
 */
void
save_ply_view (View::Ptr view, std::string const& filename,
    std::string const& depthmap, std::string const& confidence,
    std::string const& color_image);

/**
 * Stores a scanalyze compatible XF file with camera
 * transformation from camera to world coordinates.
 */
void
save_xf_file (std::string const& filename, CameraInfo const& camera);

/**
 * Stores a scanalyze compatible XF file with a given camera to
 * world matrix (16 float entries).
 */
void
save_xf_file (std::string const& filename, float const* ctw);

/* ---------------------------------------------------------------- */

/** PLY vertex element properties. */
enum PLYVertexProperty
{
    PLY_V_FLOAT_X,
    PLY_V_FLOAT_Y,
    PLY_V_FLOAT_Z,
    PLY_V_DOUBLE_X,
    PLY_V_DOUBLE_Y,
    PLY_V_DOUBLE_Z,
    PLY_V_FLOAT_NX,
    PLY_V_FLOAT_NY,
    PLY_V_FLOAT_NZ,
    PLY_V_UINT8_R,
    PLY_V_UINT8_G,
    PLY_V_UINT8_B,
    PLY_V_FLOAT_R,
    PLY_V_FLOAT_G,
    PLY_V_FLOAT_B,
    PLY_V_FLOAT_U,
    PLY_V_FLOAT_V,
    PLY_V_FLOAT_CONF,
    PLY_V_FLOAT_VALUE,
	PLY_V_VIEW_ID,
    PLY_V_IGNORE_FLOAT,
    PLY_V_IGNORE_DOUBLE,
    PLY_V_IGNORE_UINT32,
	PLY_V_IGNORE_UINT8,
};

/** PLY face element properties. */
enum PLYFaceProperty
{
    PLY_F_VERTEX_INDICES,
    PLY_F_IGNORE_UINT32,
    PLY_F_IGNORE_UINT8,
    PLY_F_IGNORE_FLOAT
};

/** PLY data encoding formats. */
enum PLYFormat
{
    PLY_ASCII,
    PLY_BINARY_LE,
    PLY_BINARY_BE,
    PLY_UNKNOWN
};

/** Reads a value from the input stream given the PLY format. */
template <typename T>
T
ply_read_value (std::istream& input, PLYFormat format);

MVE_GEOM_NAMESPACE_END
MVE_NAMESPACE_END

#endif /* MVE_PLY_FILE_HEADER */
