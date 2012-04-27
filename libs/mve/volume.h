/*
 * Regular volume representation.
 * Written by Simon Fuhrmann.
 */
#ifndef MVE_VOLUME_HEADER
#define MVE_VOLUME_HEADER

#include <vector>
#include <limits>

#include "math/vector.h"
#include "util/refptr.h"

#include "defines.h"

MVE_NAMESPACE_BEGIN

template <typename T> class Volume;
typedef Volume<float> FloatVolume;

/**
 * A volume with regular grid layout.
 */
template <typename T>
class Volume
{
public:
    typedef util::RefPtr<Volume<T> > Ptr;
    typedef util::RefPtr<Volume<T> const> ConstPtr;
    typedef std::vector<T> Voxels;

private:
    std::size_t w;
    std::size_t h;
    std::size_t d;
    Voxels data;

public:
    Volume (void);
    static Ptr create (std::size_t w, std::size_t h, std::size_t d);

    /** Allocates new volume space, clearing previous contents. */
    void allocate (std::size_t w, std::size_t h, std::size_t d);

    /** Returns data vector for the volume. */
    Voxels& get_data (void);
    /** Returns data vector for the volume. */
    Voxels const& get_data (void) const;

    /** Returns width of the image. */
    std::size_t width (void) const;
    /** Returns height of the image. */
    std::size_t height (void) const;
    /** Returns depth of the image. */
    std::size_t depth (void) const;
};

/* ---------------------------------------------------------------- */

class VolumeMCAccessor
{
private:
    std::size_t iter;

public:
    mve::FloatVolume::Ptr vol;
    float sdf[8];
    std::size_t vid[8];
    math::Vec3f pos[8];

public:
    VolumeMCAccessor (void);
    bool next (void);
};

/* ---------------------------------------------------------------- */

/* Currently only for float volumes. */
class VolumeMTAccessor
{
private:
    std::size_t iter;
    math::Vec3f cube_pos[8];
    std::size_t cube_vids[8];

public:
    mve::FloatVolume::Ptr vol;
    float sdf[4];
    std::size_t vid[4];
    math::Vec3f pos[4];

public:
    VolumeMTAccessor (void);
    bool next (void);
    void load_new_cube (void);
};

/* -------------------------- Implementation ---------------------- */

template <typename T>
inline
Volume<T>::Volume (void)
    : w(0), h(0), d(0)
{
}

template <typename T>
inline typename Volume<T>::Ptr
Volume<T>::create (std::size_t w, std::size_t h, std::size_t d)
{
    typename Volume<T>::Ptr v(new Volume());
    v->allocate(w, h, d);
    return v;
}

template <typename T>
inline void
Volume<T>::allocate (std::size_t w, std::size_t h, std::size_t d)
{
    this->w = w;
    this->h = h;
    this->d = d;
    this->data.resize(w * h * d);
}

template <typename T>
inline typename Volume<T>::Voxels&
Volume<T>::get_data (void)
{
    return this->data;
}

template <typename T>
inline typename Volume<T>::Voxels const&
Volume<T>::get_data (void) const
{
    return this->data;
}

template <typename T>
inline std::size_t
Volume<T>::width (void) const
{
    return this->w;
}

template <typename T>
inline std::size_t
Volume<T>::height (void) const
{
    return this->h;
}

template <typename T>
inline std::size_t
Volume<T>::depth (void) const
{
    return this->d;
}

MVE_NAMESPACE_END

#endif /* MVE_VOLUME_HEADER */
