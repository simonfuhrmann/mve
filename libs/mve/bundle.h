/*
 * Data structure for SfM bundles with cameras and feature points.
 * Written by Simon Fuhrmann.
 */

#ifndef MVE_BUNDLE_HEADER
#define MVE_BUNDLE_HEADER

#include <vector>

#include "util/ref_ptr.h"
#include "mve/camera.h"
#include "mve/mesh.h"
#include "mve/defines.h"

MVE_NAMESPACE_BEGIN

/**
 * A simple data structure to represent bundle files.
 * A bundle file contains a set of cameras and a 3D feature points.
 * Every feature is associated with cameras that observe the feature.
 */
class Bundle
{
public:
    /**
     * Representation of a 2D feature.
     */
    struct Feature2D
    {
        int view_id;
        int feature_id;
        float pos[2];
    };

    /**
     * Representation of a 3D feature with position and color. Every feature
     * also corresponds to a set of views form which it is seen.
     */
    struct Feature3D
    {
        float pos[3];
        float color[3];
        std::vector<Feature2D> refs;

        bool contains_view_id (int id) const;
    };

public:
    typedef util::RefPtr<Bundle> Ptr;
    typedef util::RefPtr<Bundle const> ConstPtr;
    typedef std::vector<CameraInfo> Cameras;
    typedef std::vector<Feature3D> Features;

public:
    static Ptr create (void);

    /** Returns all (possibly invalid) cameras (check focal length). */
    Cameras const& get_cameras (void) const;
    /** Returns all (possibly invalid) cameras (check focal length). */
    Cameras& get_cameras (void);
    /** Returns the list of 3D features points. */
    Features const& get_features (void) const;
    /** Returns the list of 3D features points. */
    Features& get_features (void);
    /** Returns the number of bytes required by this bundle. */
    std::size_t get_byte_size (void) const;
    /** Returns the number of cameras including invalid cameras. */
    std::size_t get_num_cameras (void) const;
    /** Returns the number of cameras excluding inavlid cameras. */
    std::size_t get_num_valid_cameras (void) const;
    /** Returns all 3D features as colored set of points. */
    TriangleMesh::Ptr get_features_as_mesh (void) const;

protected:
    Bundle (void);

private:
    Cameras cameras;
    Features features;
};

/* -------------------------------------------------------------- */

inline
Bundle::Bundle (void)
{
}

inline Bundle::Ptr
Bundle::create (void)
{
    return Ptr(new Bundle);
}

inline Bundle::Cameras const&
Bundle::get_cameras (void) const
{
    return this->cameras;
}

inline Bundle::Cameras&
Bundle::get_cameras (void)
{
    return this->cameras;
}

inline Bundle::Features const&
Bundle::get_features (void) const
{
    return this->features;
}

inline Bundle::Features&
Bundle::get_features (void)
{
    return this->features;
}

MVE_NAMESPACE_END

#endif /* MVE_BUNDLE_HEADER */
