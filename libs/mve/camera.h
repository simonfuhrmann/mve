/*
 * The MVE camera representation.
 * Written by Simon Fuhrmann.
 */

#ifndef MVE_CAMERA_HEADER
#define MVE_CAMERA_HEADER

#include <string>

#include "mve/defines.h"

MVE_NAMESPACE_BEGIN

/**
 * Per-view camera information with various helper functions.
 * An invalid camera is indicated with focal length set to 0.0f.
 */
struct CameraInfo
{
public:
    /**
     * Creates a new camera and invalidates it (sets 'flen' to 0.0f).
     */
    CameraInfo (void);

    /**
     * Stores camera position 3-vector into array pointed to by pos.
     * This can be thought of as the camera to world translation.
     * The position is calculated with: -R^T * t.
     */
    void fill_camera_pos (float* pos) const;

    /**
     * Stores the camera translation 3-vector into array pointed to by pos.
     * This can be thought of as the world to camera translation.
     * This is identical to the translation stored in the camera.
     */
    void fill_camera_translation (float* trans) const;

    /**
     * Stores the (normalized) viewing direction in world coordinates
     * into the array pointed to by 'viewdir'.
     */
    void fill_viewing_direction (float* viewdir) const;

    /**
     * Stores world to camera 4x4 matrix in array pointed to by mat.
     */
    void fill_world_to_cam (float* mat) const;

    /**
     * Stores camera to world 4x4 matrix in array pointed to by mat.
     */
    void fill_cam_to_world (float* mat) const;

    /**
     * Stores the world to camera 3x3 rotation matrix in mat.
     * This is identical to the rotation stored in the camera.
     */
    void fill_world_to_cam_rot (float* mat) const;

    /**
     * Stores the camera to world 3x3 rotation matrix in mat.
     * This is identical to the transposed rotation stored in the camera.
     */
    void fill_cam_to_world_rot (float* mat) const;

    /**
     * Initializes 'rot' and 'trans' members using the 4x4 matrix 'mat'.
     */
    void set_transformation (float const* mat);

    /**
     * Stores the 3x3 calibration (or projection) matrix (K-matrix in
     * Hartley, Zisserman). The matrix projects a point in camera coordinates
     * to the image plane with dimensions 'width' and 'height'. The convention
     * is that the camera looks along the positive z-axis. To obtain the
     * pixel coordinates after projection, 0.5 must be subtracted from the
     * coordinates.
     *
     * If the dimensions of the image are unknown, the generic projection
     * matrix with w=1 and h=1 can be used. Image coordinates x' and y' for
     * image size w and h are then computed as follows:
     *
     * For w > h:  x' = x * w  and  y' = y * w - (w - h) / 2.
     * For h > w:  x' = x * h - (h - w) / 2  and  y' = y * h.
     */
    void fill_calibration (float* mat, float width, float height) const;

    /**
     * Stores 3x3 inverse calibration (or inverse projection) matrix.
     * The matrix projects a point (x, y, 1) from the image plane into
     * the camera coordinate system. Note that 0.5 must be added to the pixel
     * coordinates x and y before projection.
     */
    void fill_inverse_calibration (float* mat,
        float width, float height) const;

    /**
     * Generates space separated list of floats for extrinsic parameters.
     * The list is in format: t1 t2 t3 r1 ... r9.
     */
    std::string to_ext_string (void) const;

    /**
     * Initializes extrinsic camera parameters from string.
     * The string must be in the format 'to_ext_string' produces.
     */
    void from_ext_string (std::string const& str);

    /**
     * Generates space separated list of floats for intrinsic parameters.
     * The list is in format: fl rd1 rd2 pa ppx ppy (see Wiki for details).
     */
    std::string to_int_string (void) const;

    /**
     * Initializes intrinsic camera parameters from string.
     * The string must be in the format 'to_int_string' produces.
     */
    void from_int_string (std::string const& str);

    /**
     * Prints debug information to stdout.
     */
    void debug_print (void) const;

public:
    /* Intrinsic camera parameters. */

    /** Focal length. */
    float flen;
    /** Principal point in x- and y-direction. */
    float ppoint[2];
    /** Pixel aspect ratio pixel_width / pixel_height. */
    float paspect;
    /** Image distortion parameters. */
    float dist[2];

    /* Extrinsic camera parameters. */

    /** Camera translation vector. Camera position p = -ROT^T * trans. */
    float trans[3];
    /** Camera rotation which transforms from world to cam. */
    float rot[9];
};

MVE_NAMESPACE_END

#endif /* MVE_CAMERA_HEADER */
