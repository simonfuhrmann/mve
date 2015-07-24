/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef MVE_VOLUME_HEADER
#define MVE_VOLUME_HEADER

#include <vector>
#include <limits>
#include <memory>

#include "math/vector.h"
#include "mve/defines.h"

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
    typedef std::shared_ptr<Volume<T> > Ptr;
    typedef std::shared_ptr<Volume<T> const> ConstPtr;
    typedef std::vector<T> Voxels;

public:
    Volume (void);
    static Ptr create (int width, int height, int depth);

    /** Allocates new volume space, clearing previous contents. */
    void allocate (int width, int height, int depth);

    /** Returns data vector for the volume. */
    Voxels& get_data (void);
    /** Returns data vector for the volume. */
    Voxels const& get_data (void) const;

    /** Returns width of the image. */
    int width (void) const;
    /** Returns height of the image. */
    int height (void) const;
    /** Returns depth of the image. */
    int depth (void) const;

private:
    int w;
    int h;
    int d;
    Voxels data;
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
    math::Vec3f color[8];

public:
    VolumeMCAccessor (void);
    bool next (void);
    bool has_colors (void) const;
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
Volume<T>::create (int width, int height, int depth)
{
    typename Volume<T>::Ptr v(new Volume());
    v->allocate(width, height, depth);
    return v;
}

template <typename T>
inline void
Volume<T>::allocate (int width, int height, int depth)
{
    this->w = width;
    this->h = height;
    this->d = depth;
    this->data.resize(width * height * depth);
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
inline int
Volume<T>::width (void) const
{
    return this->w;
}

template <typename T>
inline int
Volume<T>::height (void) const
{
    return this->h;
}

template <typename T>
inline int
Volume<T>::depth (void) const
{
    return this->d;
}

MVE_NAMESPACE_END

#endif /* MVE_VOLUME_HEADER */
