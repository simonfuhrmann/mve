#include <algorithm>

#include "camera.h"
#include "imagetools.h"

MVE_NAMESPACE_BEGIN
MVE_IMAGE_NAMESPACE_BEGIN

/*
 * ----------------------- Image conversion -----------------------
 */

FloatImage::Ptr
byte_to_float_image (ByteImage::ConstPtr image)
{
    FloatImage::Ptr img(FloatImage::create());
    img->allocate(image->width(), image->height(), image->channels());
    for (int i = 0; i < image->get_value_amount(); ++i)
    {
        float value = (float)image->at(i) / 255.0f;
        img->at(i) = std::min(1.0f, std::max(0.0f, value));
    }
    return img;
}

/* ---------------------------------------------------------------- */

DoubleImage::Ptr
byte_to_double_image (ByteImage::ConstPtr image)
{
    DoubleImage::Ptr img(DoubleImage::create());
    img->allocate(image->width(), image->height(), image->channels());
    for (int i = 0; i < image->get_value_amount(); ++i)
    {
        double value = (double)image->at(i) / 255.0;
        img->at(i) = std::min(1.0, std::max(0.0, value));
    }
    return img;
}

/* ---------------------------------------------------------------- */

ByteImage::Ptr
float_to_byte_image (FloatImage::ConstPtr image, float vmin, float vmax)
{
    ByteImage::Ptr img(ByteImage::create());
    img->allocate(image->width(), image->height(), image->channels());
    for (int i = 0; i < image->get_value_amount(); ++i)
    {
        float value = std::min(vmax, std::max(vmin, image->at(i)));
        value = 255.0f * (value - vmin) / (vmax - vmin);
        img->at(i) = (uint8_t)(value + 0.5f);
    }
    return img;
}

/* ---------------------------------------------------------------- */

ByteImage::Ptr
double_to_byte_image (DoubleImage::ConstPtr image, double vmin, double vmax)
{
    ByteImage::Ptr img(ByteImage::create());
    img->allocate(image->width(), image->height(), image->channels());
    for (int i = 0; i < image->get_value_amount(); ++i)
    {
        double value = std::min(vmax, std::max(vmin, image->at(i)));
        value = 255.0 * (value - vmin) / (vmax - vmin);
        img->at(i) = (uint8_t)(value + 0.5);
    }
    return img;
}

/* ---------------------------------------------------------------- */

ByteImage::Ptr
int_to_byte_image (IntImage::ConstPtr image)
{
    ByteImage::Ptr img(ByteImage::create());
    img->allocate(image->width(), image->height(), image->channels());
    for (int i = 0; i < image->get_value_amount(); ++i)
        img->at(i) = math::algo::clamp(std::abs(image->at(i)), 0, 255);

    return img;
}

/* ---------------------------------------------------------------- */

void
float_image_normalize (FloatImage::Ptr image)
{
    float vmin = std::numeric_limits<float>::max();
    float vmax = std::numeric_limits<float>::min();

    int values = image->get_value_amount();
    for (int i = 0; i < values; ++i)
    {
        vmin = std::min(vmin, image->at(i));
        vmax = std::max(vmax, image->at(i));
    }

    for (int i = 0; i < values; ++i)
        image->at(i) = (image->at(i) - vmin) / (vmax - vmin);
}

/* ---------------------------------------------------------------- */

void
gamma_correct (ByteImage::Ptr image, float power)
{
    uint8_t lookup[256];
    for (int i = 0; i < 256; ++i)
        lookup[i] = static_cast<uint8_t>(std::pow(i / 255.0f, power) * 255.0f + 0.5f);
    for (int i = 0; i < image->get_value_amount(); ++i)
        image->at(i) = lookup[image->at(i)];
}

MVE_IMAGE_NAMESPACE_END
MVE_NAMESPACE_END
