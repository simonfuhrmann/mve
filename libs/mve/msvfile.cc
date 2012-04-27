#include <iostream>
#include <iomanip>
#include <fstream>
#include <cerrno>
#include <cstring>

#include "util/exception.h"
#include "image.h"
#include "msvfile.h"

MVE_NAMESPACE_BEGIN

void
MsvFile::read_headers_intern (std::ifstream& handle)
{
    handle >> headers.version >> headers.cbit
        >> headers.left >> headers.top >> headers.right
        >> headers.bottom >> headers.channels
        >> headers.bytesperpixel;

    handle.setf(std::ios::hex, std::ios::basefield);
    handle.setf(std::ios::showbase);
    handle >> headers.vispixfmt;
    handle.get();
}

/* ------------------------------------------------------------ */

FloatImage::Ptr
MsvFile::load_all (void)
{
    if (this->filename.empty())
        throw util::Exception("MsvFile: Empty filename given");

    std::cout << "Loading all channels from MSV file... " << std::endl;

    std::ifstream file(this->filename.c_str(), std::ios::in | std::ios::binary);
    if (!file.good())
        throw util::Exception("Error opening MSV file: ", std::strerror(errno));

    this->read_headers_intern(file);

    std::size_t width = this->headers.right - this->headers.left;
    std::size_t height = this->headers.bottom - this->headers.top;
    std::size_t chans = this->headers.channels;
    FloatImage::Ptr image = FloatImage::create();
    image->allocate(width, height, chans);

    file.read(image->get_byte_pointer(), image->get_byte_size());
    file.close();

    return image;
}

/* ------------------------------------------------------------ */

FloatImage::Ptr
MsvFile::load_channel (std::size_t channel)
{
    if (this->filename.empty())
        throw util::Exception("MsvFile: Empty filename given");

    std::cout << "Loading channel " << channel << " from MSV file... "
        << std::endl;

    std::ifstream file(this->filename.c_str(), std::ios::in | std::ios::binary);
    if (!file.good())
        throw util::Exception("Error opening MSV file: ", std::strerror(errno));

    this->read_headers_intern(file);

    std::size_t width = this->headers.right - this->headers.left;
    std::size_t height = this->headers.bottom - this->headers.top;
    FloatImage::Ptr image = FloatImage::create();
    image->allocate(width, height, 1);

    for (std::size_t y = 0; y < height; ++y)
        for (std::size_t x = 0; x < width; ++x)
            for (std::size_t c = 0; c < MSV_CHANNEL_AMOUNT; ++c)
            {
                float tmp;
                file.read((char*)&tmp, 4);
                if (c == channel)
                    image->at(x, y, 0) = tmp;
            }

    file.close();

    return image;
}


/* ------------------------------------------------------------ */

FloatImage::Ptr
MsvFile::load_depthmap (void)
{
    return this->load_channel(MSV_CHANNEL_D);
}

/* ------------------------------------------------------------ */

FloatImage::Ptr
MsvFile::load_confidence (void)
{
    return this->load_channel(MSV_CHANNEL_C);
}

/* ------------------------------------------------------------ */

ByteImage::Ptr
MsvFile::load_rgb_image (void)
{
    if (this->filename.empty())
        throw util::Exception("MsvFile: Empty filename given");

    std::cout << "Loading RGB image from MSV file..." << std::endl;

    std::ifstream file(filename.c_str(), std::ios::in | std::ios::binary);
    if (!file.good())
        throw util::Exception("Error opening MSV file: ", std::strerror(errno));

    this->read_headers_intern(file);

    std::size_t width = this->headers.right - this->headers.left;
    std::size_t height = this->headers.bottom - this->headers.top;
    ByteImage::Ptr image = ByteImage::create();
    image->allocate(width, height, 3);

    for (std::size_t y = 0; y < height; ++y)
        for (std::size_t x = 0; x < width; ++x)
            for (std::size_t c = 0; c < MSV_CHANNEL_AMOUNT; ++c)
            {
                float tmp;
                file.read((char*)&tmp, 4);
                tmp = tmp * 255.0f;
                tmp = std::max(tmp, 0.0f);
                tmp = std::min(255.0f, tmp);
                unsigned char value = (unsigned char)tmp;

                if (c == MSV_CHANNEL_R)
                    image->at(x, y, 0) = value;
                if (c == MSV_CHANNEL_G)
                    image->at(x, y, 1) = value;
                if (c == MSV_CHANNEL_B)
                    image->at(x, y, 2) = value;
            }

    file.close();

    return image;
}

/* ------------------------------------------------------------ */

void
MsvFile::read_headers (void)
{
    if (this->filename.empty())
        throw util::Exception("MsvFile: Empty filename given");

    std::ifstream file(this->filename.c_str(), std::ios::in | std::ios::binary);
    if (!file.good())
        throw util::Exception("Error opening MSV file: ", std::strerror(errno));
    this->read_headers_intern(file);
    file.close();
}

/* ------------------------------------------------------------ */

void
MsvFile::debug_print_headers (void)
{
    std::cout << "MVS file information (" << this->filename << ")" << std::endl;
    std::cout << "\tVersion: " << this->headers.version << std::endl;
    std::cout << "\tcbit: " << this->headers.cbit << std::endl;
    std::cout << "\tleft: " << this->headers.left << std::endl;
    std::cout << "\ttop: " << this->headers.top << std::endl;
    std::cout << "\tright: " << this->headers.right << std::endl;
    std::cout << "\tbottom: " << this->headers.bottom << std::endl;
    std::cout << "\tchannels: " << this->headers.channels << std::endl;
    std::cout << "\tbytes/Pixel: " << this->headers.bytesperpixel << std::endl;
    std::cout << "\tvispixfmt: " << this->headers.vispixfmt << std::endl;
}

MVE_NAMESPACE_END
