/*
 * A reader and writer for bundle files.
 * Written by Simon Fuhrmann.
 *
 * Photosynther resources: http://synthexport.codeplex.com/
 * Noah bundler resources: http://phototour.cs.washington.edu/bundler/
 */

#ifndef MVE_BUNDLE_FILE_HEADER
#define MVE_BUNDLE_FILE_HEADER

#include <string>
#include <vector>

#include "util/refptr.h"

#include "defines.h"
#include "camera.h"
#include "trianglemesh.h"

MVE_NAMESPACE_BEGIN

/** Struct that represents a point reference. */
struct FeaturePointRef
{
    int img_id;
    int feature_id;
    float error;
};

/** Struct that represents a feature point (e.g. SIFT). */
struct FeaturePoint
{
    float pos[3];
    unsigned char color[3];
    std::vector<FeaturePointRef> refs;

    bool contains_view_id (std::size_t id) const;
};

/** Identification of the detected bundler format. */
enum BundleFormat
{
    BUNDELR_UNKNOWN,
    BUNDLER_PHOTOSYNTHER,
    BUNDLER_NOAHBUNDLER
};

/**
 * Parser for Photosynther and Noah bundle files.
 * Supported bundle files are those with "drews 1.0" (Photosynther)
 * or "# Bundle file v0.3" (Noah Snavely) in the first line.
 *
 * Note that this class gives direct access to the data in the bundle file,
 * e.g. radial distortion parameters in the camera must be interpreted
 * appropriately depending on the software that created the values, and
 * interpretation of the world coordinates (i.e. left-handed, right-handed)
 * is up to the clients of this class. The image IDs are relative to the
 * input images to the bundler software and must be interpreted accordingly.
 */
class BundleFile
{
public:
    typedef util::RefPtr<BundleFile> Ptr;
    typedef util::RefPtr<BundleFile const> ConstPtr;
    typedef std::vector<CameraInfo> BundleCameras;
    typedef std::vector<FeaturePoint> FeaturePoints;

private:
    std::string version;
    BundleCameras cameras;
    FeaturePoints points;
    BundleFormat format;
    std::size_t num_valid_cams;

private:
    void read_bundle_intern (std::string const& filename);

public:
    BundleFile (void);

    /** Creates a smart pointered instance. */
    static BundleFile::Ptr create (void);

    /**
     * Parses a bundle file and loads it into memory.
     * The format is detected according to the first line in the file.
     */
    void read_bundle (std::string const& filename);

    /**
     * Writes the memory state to a file.
     * The output file is always in Photosynther format.
     */
    void write_bundle (std::string const& filename);

    /** Releases all data. */
    void clear (void);

    /**
     * Deletes a camera from the bundle file.
     * After deletion, the indices in the camera vector DON'T change.
     * Deletion is done by setting the camera to invalid. Also feature points
     * that reference the deleted camera are modified to exclude that
     * camera. Note that this can lead to features not seen by any camera.
     */
    void delete_camera (std::size_t index);

    /** Exports all points (SIFT features) from the bundle to PLY. */
    void write_points_to_ply (std::string const& filename);

    /** Returns the version (the first line of the file). */
    std::string const& get_version (void) const;

    /** Provides access to the cameras. */
    BundleCameras const& get_cameras (void) const;
    /** Returns the number of feature points. */
    FeaturePoints const& get_points (void) const;
    /** Provides access to the cameras. */
    BundleCameras& get_cameras (void);
    /** Returns the list of feature points. */
    FeaturePoints& get_points (void);

    /** Returns the amount of cameras. */
    std::size_t get_num_cameras (void) const;
    /** Returns the amount of valid cameras. */
    std::size_t get_num_valid_cameras (void) const;

    /** Returns the points as mesh (colored points without faces). */
    TriangleMesh::Ptr get_points_mesh (void) const;

    /** Returns the detected format of the bundle file. */
    BundleFormat get_format (void) const;

    /** Returns the consumed amount of memory in bytes. */
    std::size_t get_byte_size (void) const;
};

/* -------------------------------------------------------------- */

inline
BundleFile::BundleFile (void)
    : num_valid_cams(0)
{
}

inline BundleFile::Ptr
BundleFile::create (void)
{
    return Ptr(new BundleFile);
}

inline std::string const&
BundleFile::get_version (void) const
{
    return this->version;
}

inline BundleFile::BundleCameras const&
BundleFile::get_cameras (void) const
{
    return this->cameras;
}

inline BundleFile::FeaturePoints const&
BundleFile::get_points (void) const
{
    return this->points;
}

inline BundleFile::BundleCameras&
BundleFile::get_cameras (void)
{
    return this->cameras;
}

inline BundleFile::FeaturePoints&
BundleFile::get_points (void)
{
    return this->points;
}

inline std::size_t
BundleFile::get_num_cameras (void) const
{
    return this->cameras.size();
}

inline std::size_t
BundleFile::get_num_valid_cameras (void) const
{
    return this->num_valid_cams;
}

inline void
BundleFile::clear (void)
{
    this->version.clear();
    this->cameras.clear();
    this->points.clear();
}

inline BundleFormat
BundleFile::get_format (void) const
{
    return this->format;
}

MVE_NAMESPACE_END

#endif /* MVE_BUNDLE_FILE_HEADER */
