/*
 * Light-weight image class for arbitrary size and value types.
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
 * Although this class is heavily used in the View class, it can be used as
 * stand-alone class for image loading, storing and processing.
 *
 * Image data is stored in interleaved order, i.e. "RGBRGB..."
 * instead of planar order "RR..GG..BB..".
 */
template <typename T>
class Image : public TypedImageBase<T>
{
public:
    typedef util::RefPtr<Image<T> > Ptr;
    typedef util::RefPtr<Image<T> const> ConstPtr;
    typedef std::vector<T> ImageData;

public:
    /** Default ctor creates an empty image. */
    Image (void);

    /** Allocating ctor. */
    Image (std::size_t width, std::size_t height, std::size_t channels);

    /** Template copy ctor converts from another image. */
    template <typename O>
    Image (Image<O> const& other);

    /** Smart pointer image constructor. */
    static typename Image<T>::Ptr create (void);

    /** Allocating smart pointer image constructor. */
    static typename Image<T>::Ptr create (std::size_t width,
        std::size_t height, std::size_t channels);

    /** Smart pointer image copy constructor. */
    static typename Image<T>::Ptr create (Image<T> const& other);

    // TODO: Test features
    /** Adds 'amount' channels to the back with default value 'value'. */
    void add_channels (std::size_t amount, T const& value = T(0));
    /** Swaps channels 'c1' and 'c2'. */
    void swap_channels (std::size_t c1, std::size_t c2);
    /** Copies channel from src to dest. "-1" for dest creates new channel. */
    void copy_channel (std::size_t src, std::size_t dest);
    /** Deletes a channel from the image. */
    void delete_channel (std::size_t channel);

    /** Linear indexing of image data. */
    T const& at (std::size_t index) const;
    /** Linear indexing of channel data. */
    T const& at (std::size_t index, std::size_t channel) const;
    /** 2D indexing of image data, more expensive. */
    T const& at (std::size_t x, std::size_t y, std::size_t channel) const;

    /** Linear indexing of image data. */
    T& at (std::size_t index);
    /** Linear indexing of channel data. */
    T& at (std::size_t index, std::size_t channel);
    /** 2D indexing of image data, more expensive. */
    T& at (std::size_t x, std::size_t y, std::size_t channel);

    /** Linear interpolation (more expensive) for a single color channel. */
    T linear_at (float x, float y, std::size_t channel) const;

    /**
     * Linear interpolation (more expensive) for all color channels.
     * The method generates one value for each color channel and places
     * the result in the buffer provided by 'px'.
     */
    void linear_at (float x, float y, T* px) const;

    // TODO operators for data access (operators & refptr?)
    T& operator[] (std::size_t index);
    T const& operator[] (std::size_t index) const;

    T const& operator() (std::size_t index) const;
    T const& operator() (std::size_t index, std::size_t channel) const;
    T const& operator() (std::size_t x, std::size_t y, std::size_t channel) const;
    T& operator() (std::size_t index);
    T& operator() (std::size_t index, std::size_t channel);
    T& operator() (std::size_t x, std::size_t y, std::size_t channel);
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
Image<T>::Image (std::size_t width, std::size_t height, std::size_t channels)
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
Image<T>::create (std::size_t width, std::size_t height, std::size_t channels)
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
Image<T>::add_channels (std::size_t num_channels, T const& value)
{
    std::size_t old_chans = this->channels();
    std::size_t src_ptr = this->get_value_amount();

    this->resize(this->width(), this->height(),
        this->channels() + num_channels);

    std::size_t dest_ptr = this->get_value_amount();

    while (dest_ptr > 0) // Exits if dest_ptr == 0
    {
        for (std::size_t i = 0; i < num_channels; ++i)
        {
            dest_ptr -= 1;
            this->at(dest_ptr) = value;
        }
        for (std::size_t i = 0; i < old_chans; ++i)
        {
            dest_ptr -= 1;
            src_ptr -= 1;
            this->at(dest_ptr) = this->at(src_ptr);
        }
    }
}

template <typename T>
inline void
Image<T>::swap_channels (std::size_t c1, std::size_t c2)
{
    for (std::size_t i = 0; i < this->data.size(); i += this->channels())
        std::swap(this->data[i + c1], this->data[i + c2]);
}

template <typename T>
/*inline*/ void
Image<T>::copy_channel (std::size_t src, std::size_t dest)
{
    if (src == dest)
        return;

    if (dest == (std::size_t)-1)
    {
      dest = this->channels();
      this->add_channels(1);
    }

    for (std::size_t i = 0; i < this->data.size(); i += this->channels())
        this->data[i + dest] = this->data[i + src];
}

template <typename T>
/*inline*/ void
Image<T>::delete_channel (std::size_t chan)
{
    if (chan >= this->channels())
        return;

    std::size_t dest_ptr = 0;
    std::size_t src_ptr = 0;
    while (src_ptr < this->get_value_amount())
    {
        if (src_ptr % this->channels() == chan)
            src_ptr += 1;
        else
        {
            this->at(dest_ptr) = this->at(src_ptr);
            src_ptr += 1;
            dest_ptr += 1;
        }
    }
    this->resize(this->width(), this->height(), this->channels() - 1);
}

template <typename T>
inline T const&
Image<T>::at (std::size_t index) const
{
    return this->data[index];
}

template <typename T>
inline T const&
Image<T>::at (std::size_t index, std::size_t channel) const
{
    std::size_t off = index * this->channels() + channel;
    return this->data[off];
}

template <typename T>
inline T const&
Image<T>::at (std::size_t x, std::size_t y, std::size_t channel) const
{
    std::size_t off = channel
        + x * this->channels()
        + y * this->channels() * this->width();
    return this->data[off];
}

template <typename T>
inline T&
Image<T>::at (std::size_t index)
{
    return this->data[index];
}

template <typename T>
inline T&
Image<T>::at (std::size_t index, std::size_t channel)
{
    std::size_t off = index * this->channels() + channel;
    return this->data[off];
}

template <typename T>
inline T&
Image<T>::at (std::size_t x, std::size_t y, std::size_t channel)
{
    std::size_t off = channel
        + x * this->channels()
        + y * this->channels() * this->width();
    return this->data[off];
}

template <typename T>
inline T&
Image<T>::operator[] (std::size_t index)
{
    return this->data[index];
}

template <typename T>
inline T const&
Image<T>::operator[] (std::size_t index) const
{
    return this->data[index];
}

template <typename T>
inline T const&
Image<T>::operator() (std::size_t index) const
{
    return this->at(index);
}

template <typename T>
inline T const&
Image<T>::operator() (std::size_t index, std::size_t channel) const
{
    return this->at(index, channel);
}

template <typename T>
inline T const&
Image<T>::operator() (std::size_t x, std::size_t y, std::size_t channel) const
{
    return this->at(x, y, channel);
}

template <typename T>
inline T&
Image<T>::operator() (std::size_t index)
{
    return this->at(index);
}

template <typename T>
inline T&
Image<T>::operator() (std::size_t index, std::size_t channel)
{
    return this->at(index, channel);
}

template <typename T>
inline T&
Image<T>::operator() (std::size_t x, std::size_t y, std::size_t channel)
{
    return this->at(x, y, channel);
}

template <typename T>
T
Image<T>::linear_at (float x, float y, std::size_t channel) const
{
    x = std::max(0.0f, std::min((float)(this->w - 1), x));
    y = std::max(0.0f, std::min((float)(this->h - 1), y));
    std::size_t x1 = (std::size_t)std::floor(x);
    std::size_t x2 = (std::size_t)std::ceil(x);
    std::size_t y1 = (std::size_t)std::floor(y);
    std::size_t y2 = (std::size_t)std::ceil(y);
    float wx2 = x - std::floor(x);
    float wx1 = 1.0f - wx2;
    float wy2 = y - std::floor(y);
    float wy1 = 1.0f - wy2;
    std::size_t row1 = y1 * this->w * this->c;
    std::size_t row2 = y2 * this->w * this->c;
    return math::algo::interpolate<T>(at(row1 + x1 * this->c + channel),
        at(row1 + x2 * this->c + channel),
        at(row2 + x1 * this->c + channel),
        at(row2 + x2 * this->c + channel),
        wx1 * wy1, wx2 * wy1, wx1 * wy2, wx2 * wy2);
}

template <typename T>
void
Image<T>::linear_at (float x, float y, T* px) const
{
    /* Determine the four pixels and weights. */
    std::size_t pos[4];
    float w[4];
    {
        x = std::max(0.0f, std::min((float)(this->w - 1), x));
        y = std::max(0.0f, std::min((float)(this->h - 1), y));

        std::size_t floor_x = (std::size_t)x;
        std::size_t floor_y = (std::size_t)y;
        std::size_t floor_xp1 = std::min(floor_x + 1, this->w - 1);
        std::size_t floor_yp1 = std::min(floor_y + 1, this->h - 1);

        w[1] = x - (float)floor_x;
        w[0] = 1.0f - w[1];
        w[3] = y - (float)floor_y;
        w[2] = 1.0f - w[3];

        std::size_t rowstride(this->w * this->c);
        std::size_t row1 = floor_y * rowstride;
        std::size_t row2 = floor_yp1 * rowstride;
        std::size_t col1 = floor_x * this->c;
        std::size_t col2 = floor_xp1 * this->c;

        pos[0] = row1 + col1;
        pos[1] = row1 + col2;
        pos[2] = row2 + col1;
        pos[3] = row2 + col2;
    }

    /* Copy interpolated value to output buffer. */
    for (std::size_t cc = 0; cc < this->c; ++cc)
    {
        px[cc] = math::algo::interpolate<T>
            (this->at(pos[0] + cc), this->at(pos[1] + cc),
            this->at(pos[2] + cc), this->at(pos[3] + cc),
            w[0] * w[2], w[1] * w[2], w[0] * w[3], w[1] * w[3]);
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
