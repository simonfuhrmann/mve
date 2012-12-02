/*
 * Light-weight image data structure for arbitrary size and value types.
 * Written by Simon Fuhrmann.
 */

#ifndef MVE_IMAGE_HEADER
#define MVE_IMAGE_HEADER

#include <string>
#include <vector>

#include "util/refptr.h"
#include "math/algo.h"
#include "defines.h"
#include "imagebase.h"

MVE_NAMESPACE_BEGIN

template <typename T> class Image;
typedef Image<uint8_t> ByteImage;
typedef Image<uint16_t> RawImage;
typedef Image<char> CharImage;
typedef Image<float> FloatImage;
typedef Image<double> DoubleImage;
typedef Image<int> IntImage;
//typedef Image<bool> BoolImage;

/**
 * Multi-channel image class of arbitrary but homogenous data type.
 * Image data is interleaved, i.e. "RGBRGB...", not planar "RR..GG..BB..".
 */
template <typename T>
class Image : public TypedImageBase<T>
{
public:
    typedef util::RefPtr<Image<T> > Ptr;
    typedef util::RefPtr<Image<T> const> ConstPtr;
    typedef std::vector<T> ImageData;
    typedef T ValueType;

public:
    /** Default ctor creates an empty image. */
    Image (void);

    /** Allocating ctor. */
    Image (int width, int height, int channels);

    /** Template copy ctor converts from another image. */
    template <typename O>
    Image (Image<O> const& other);

    /** Smart pointer image constructor. */
    static typename Image<T>::Ptr create (void);

    /** Allocating smart pointer image constructor. */
    static typename Image<T>::Ptr create (int width, int height, int channels);

    /** Smart pointer image copy constructor. */
    static typename Image<T>::Ptr create (Image<T> const& other);

    // TODO: Test features
    /** Adds 'amount' channels to the back with default value 'value'. */
    void add_channels (int amount, T const& value = T(0));
    /** Swaps channels 'c1' and 'c2'. */
    void swap_channels (int c1, int c2);
    /** Copies channel from src to dest. "-1" for dest creates new channel. */
    void copy_channel (int src, int dest);
    /** Deletes a channel from the image. */
    void delete_channel (int channel);

    /** Linear indexing of image data. */
    T const& at (int index) const;
    /** Linear indexing of channel data. */
    T const& at (int index, int channel) const;
    /** 2D indexing of image data, more expensive. */
    T const& at (int x, int y, int channel) const;

    /** Linear indexing of image data. */
    T& at (int index);
    /** Linear indexing of channel data. */
    T& at (int index, int channel);
    /** 2D indexing of image data, more expensive. */
    T& at (int x, int y, int channel);

    /** Linear interpolation (more expensive) for a single color channel. */
    T linear_at (float x, float y, int channel) const;

    /**
     * Linear interpolation (more expensive) for all color channels.
     * The method generates one value for each color channel and places
     * the result in the buffer provided by 'px'.
     */
    void linear_at (float x, float y, T* px) const;

    // TODO operators for data access (operators & refptr?)
    T& operator[] (int index);
    T const& operator[] (int index) const;

    T const& operator() (int index) const;
    T const& operator() (int index, int channel) const;
    T const& operator() (int x, int y, int channel) const;
    T& operator() (int index);
    T& operator() (int index, int channel);
    T& operator() (int x, int y, int channel);
};

MVE_NAMESPACE_END

/* ------------------------- Implementation ----------------------- */

#include <cmath> // for std::ceil, std::floor

MVE_NAMESPACE_BEGIN

template <typename T>
inline
Image<T>::Image (void)
{
}

template <typename T>
inline
Image<T>::Image (int width, int height, int channels)
{
    this->allocate(width, height, channels);
}

template <typename T>
template <typename O>
inline
Image<T>::Image (Image<O> const& other)
    : TypedImageBase<T>(other)
{
}

template <typename T>
inline typename Image<T>::Ptr
Image<T>::create (void)
{
    return Ptr(new Image<T>());
}

template <typename T>
inline typename Image<T>::Ptr
Image<T>::create (int width, int height, int channels)
{
    return Ptr(new Image<T>(width, height, channels));
}

template <typename T>
inline typename Image<T>::Ptr
Image<T>::create (Image<T> const& other)
{
    return Ptr(new Image<T>(other));
}

template <typename T>
/*inline*/ void
Image<T>::add_channels (int num_channels, T const& value)
{
    if (num_channels <= 0 || !this->valid())
        return;

    std::vector<T> tmp(this->w * this->h * (this->c + num_channels));
    typename std::vector<T>::iterator dest_ptr = tmp.end();
    typename std::vector<T>::const_iterator src_ptr = this->data.end();
    const int pixels = this->get_pixel_amount();
    for (int p = 0; p < pixels; ++p)
    {
        for (int i = 0; i < num_channels; ++i)
            *(--dest_ptr) = value;
        for (int i = 0; i < this->c; ++i)
            *(--dest_ptr) = *(--src_ptr);
    }

    this->c += num_channels;
    std::swap(this->data, tmp);
}

template <typename T>
inline void
Image<T>::swap_channels (int c1, int c2)
{
    if (!this->valid() || c1 == c2
        || c1 >= this->channels() || c2 >= this->channels())
        return;

    typename std::vector<T>::iterator iter1 = this->data.begin() + c1;
    typename std::vector<T>::iterator iter2 = this->data.begin() + c2;
    int pixels = this->get_pixel_amount();
    for (int i = 0; i < pixels; ++i, iter1 += this->c, iter2 += this->c)
        std::swap(*iter1, *iter2);
}

template <typename T>
/*inline*/ void
Image<T>::copy_channel (int src, int dest)
{
    if (!this->valid() || src == dest)
        return;

    if (dest < 0)
    {
        dest = this->channels();
        this->add_channels(1);
    }

    typename std::vector<T>::iterator src_iter = this->data.begin() + src;
    typename std::vector<T>::iterator dst_iter = this->data.begin() + dest;
    int pixels = this->get_pixel_amount();
    for (int i = 0; i < pixels; ++i, src_iter += this->c, dst_iter += this->c)
        *dst_iter = *src_iter;
}

template <typename T>
/*inline*/ void
Image<T>::delete_channel (int chan)
{
    if (chan < 0 || chan >= this->channels())
        return;

    typename std::vector<T>::iterator src_iter = this->data.begin();
    typename std::vector<T>::iterator dst_iter = this->data.begin();
    for (int i = 0; src_iter != this->data.end(); ++i)
    {
        if (i % this->c == chan)
            src_iter++;
        else
            *(dst_iter++) = *(src_iter++);
    }
    this->resize(this->width(), this->height(), this->channels() - 1);
}

template <typename T>
inline T const&
Image<T>::at (int index) const
{
    return this->data[index];
}

template <typename T>
inline T const&
Image<T>::at (int index, int channel) const
{
    int off = index * this->channels() + channel;
    return this->data[off];
}

template <typename T>
inline T const&
Image<T>::at (int x, int y, int channel) const
{
    int off = channel + this->channels() * (x + y * this->width());
    return this->data[off];
}

template <typename T>
inline T&
Image<T>::at (int index)
{
    return this->data[index];
}

template <typename T>
inline T&
Image<T>::at (int index, int channel)
{
    int off = index * this->channels() + channel;
    return this->data[off];
}

template <typename T>
inline T&
Image<T>::at (int x, int y, int channel)
{
    int off = channel + this->channels() * (x + y * this->width());
    return this->data[off];
}

template <typename T>
inline T&
Image<T>::operator[] (int index)
{
    return this->data[index];
}

template <typename T>
inline T const&
Image<T>::operator[] (int index) const
{
    return this->data[index];
}

template <typename T>
inline T const&
Image<T>::operator() (int index) const
{
    return this->at(index);
}

template <typename T>
inline T const&
Image<T>::operator() (int index, int channel) const
{
    return this->at(index, channel);
}

template <typename T>
inline T const&
Image<T>::operator() (int x, int y, int channel) const
{
    return this->at(x, y, channel);
}

template <typename T>
inline T&
Image<T>::operator() (int index)
{
    return this->at(index);
}

template <typename T>
inline T&
Image<T>::operator() (int index, int channel)
{
    return this->at(index, channel);
}

template <typename T>
inline T&
Image<T>::operator() (int x, int y, int channel)
{
    return this->at(x, y, channel);
}

template <typename T>
T
Image<T>::linear_at (float x, float y, int channel) const
{
    x = std::max(0.0f, std::min(static_cast<float>(this->w - 1), x));
    y = std::max(0.0f, std::min(static_cast<float>(this->h - 1), y));

    const int floor_x = static_cast<int>(x);
    const int floor_y = static_cast<int>(y);
    const int floor_xp1 = std::min(floor_x + 1, this->w - 1);
    const int floor_yp1 = std::min(floor_y + 1, this->h - 1);

    const float w1 = x - static_cast<float>(floor_x);
    const float w0 = 1.0f - w1;
    const float w3 = y - static_cast<float>(floor_y);
    const float w2 = 1.0f - w3;

    const int rowstride = this->w * this->c;
    const int row1 = floor_y * rowstride;
    const int row2 = floor_yp1 * rowstride;
    const int col1 = floor_x * this->c;
    const int col2 = floor_xp1 * this->c;

    return math::algo::interpolate<T>
        (this->at(row1 + col1 + channel), this->at(row1 + col2 + channel),
        this->at(row2 + col1 + channel), this->at(row2 + col2 + channel),
        w0 * w2, w1 * w2, w0 * w3, w1 * w3);
}

template <typename T>
void
Image<T>::linear_at (float x, float y, T* px) const
{
    x = std::max(0.0f, std::min(static_cast<float>(this->w - 1), x));
    y = std::max(0.0f, std::min(static_cast<float>(this->h - 1), y));

    const int floor_x = static_cast<int>(x);
    const int floor_y = static_cast<int>(y);
    const int floor_xp1 = std::min(floor_x + 1, this->w - 1);
    const int floor_yp1 = std::min(floor_y + 1, this->h - 1);

    const float w1 = x - static_cast<float>(floor_x);
    const float w0 = 1.0f - w1;
    const float w3 = y - static_cast<float>(floor_y);
    const float w2 = 1.0f - w3;

    const int rowstride = this->w * this->c;
    const int row1 = floor_y * rowstride;
    const int row2 = floor_yp1 * rowstride;
    const int col1 = floor_x * this->c;
    const int col2 = floor_xp1 * this->c;

    /* Copy interpolated channel values to output buffer. */
    for (int cc = 0; cc < this->c; ++cc)
    {
        px[cc] = math::algo::interpolate<T>
            (this->at(row1 + col1 + cc), this->at(row1 + col2 + cc),
            this->at(row2 + col1 + cc), this->at(row2 + col2 + cc),
            w0 * w2, w1 * w2, w0 * w3, w1 * w3);
    }
}

MVE_NAMESPACE_END

/* ---------------------------------------------------------------- */

STD_NAMESPACE_BEGIN

/** Specialization of std::swap for efficient image swapping. */
template <class T>
inline void
swap (mve::Image<T>& a, mve::Image<T>& b)
{
    a.swap(b);
}

STD_NAMESPACE_END

#endif /* MVE_IMAGE_HEADER */
