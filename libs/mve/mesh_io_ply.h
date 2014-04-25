/*
 * PLY file loading and writing functions.
 * Written by Simon Fuhrmann.
 */

#ifndef MVE_PLY_FILE_HEADER
#define MVE_PLY_FILE_HEADER

#include <string>

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
    SavePLYOptions (void)
        : format_binary(true)
        , write_vertex_colors(true)
        , write_vertex_normals(false)
        , write_vertex_confidences(false)
        , write_vertex_values(false)
        , write_face_colors(false)
        , write_face_normals(false)
        , verts_per_simplex(3) {}

    bool format_binary;
    bool write_vertex_colors;
    bool write_vertex_normals;
    bool write_vertex_confidences;
    bool write_vertex_values;
    bool write_face_colors;
    bool write_face_normals;
    unsigned int verts_per_simplex;
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
    FloatImage::ConstPtr confidence_map = FloatImage::ConstPtr(NULL),
    ByteImage::ConstPtr color_image = ByteImage::ConstPtr(NULL));

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

MVE_GEOM_NAMESPACE_END
MVE_NAMESPACE_END

#endif /* MVE_PLY_FILE_HEADER */
