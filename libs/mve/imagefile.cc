#include <algorithm>
#include <limits>
#include <fstream>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cerrno>

#include "util/endian.h"
#include "util/exception.h"
#include "util/string.h"

#include "imagefile.h"

#ifndef MVE_NO_PNG_SUPPORT
#   include <png.h>
#endif

#ifndef MVE_NO_JPEG_SUPPORT
#   include <jpeglib.h>
#endif

#ifndef MVE_NO_TIFF_SUPPORT
#   include <tiff.h>
#   include <tiffio.h>
#endif

MVE_NAMESPACE_BEGIN
MVE_IMAGE_NAMESPACE_BEGIN

/*
 * ---------------------- Loading and Saving ----------------------
 */

ByteImage::Ptr
load_file (std::string const& filename)
{
    try
    {
#ifndef MVE_NO_PNG_SUPPORT
        try
        { return load_png_file(filename); }
        catch (util::FileException& e) { throw; }
        catch (util::Exception& e) {}
#endif

#ifndef MVE_NO_JPEG_SUPPORT
        try
        { return load_jpg_file(filename); }
        catch (util::FileException& e) { throw; }
        catch (util::Exception& e) {}
#endif

#ifndef MVE_NO_TIFF_SUPPORT
        try
        { return load_tiff_file(filename); }
        catch (util::FileException& e) { throw; }
        catch (util::Exception& e) {}
#endif

        throw util::Exception("Cannot determine image format");
    }
    catch (util::FileException& e)
    {
        throw util::Exception("Error opening file: ", e.what());
    }
}

/* ---------------------------------------------------------------- */

void
save_file (ByteImage::Ptr image, std::string const& filename)
{
    using namespace util::string;
    std::string fext4 = lowercase(right(filename, 4));
    std::string fext5 = lowercase(right(filename, 5));

#ifndef MVE_NO_JPEG_SUPPORT
    if (fext4 == ".jpg" || fext5 == ".jpeg")
    {
        save_jpg_file(image, filename, 85);
        return;
    }
#endif

#ifndef MVE_NO_PNG_SUPPORT
    if (fext4 == ".png")
    {
        save_png_file(image, filename);
        return;
    }
#endif

#ifndef MVE_NO_TIFF_SUPPORT
    if (fext4 == ".tif" || fext5 == ".tiff")
    {
        save_tiff_file(image, filename);
        return;
    }
#endif

    if (fext4 == ".ppm")
    {
        save_ppm_file(image, filename);
        return;
    }

    throw util::Exception("Output filetype not supported");
}

/* ---------------------------------------------------------------- */

void
save_file (FloatImage::Ptr image, std::string const& filename)
{

    using namespace util::string;
    std::string fext4 = lowercase(right(filename, 4));
    if (fext4 == ".pfm")
    {
        save_pfm_file(image, filename);
        return;
    }

    throw util::Exception("Output filetype not supported");
}

/* ---------------------------------------------------------------- */

#ifndef MVE_NO_PNG_SUPPORT

ByteImage::Ptr
load_png_file (std::string const& filename)
{
    // TODO: use throw-safe FILE* wrapper
    FILE* fp = std::fopen(filename.c_str(), "rb");
    if (fp == NULL)
        throw util::FileException(filename, std::strerror(errno));

    /* Identify the PNG signature. */
    png_byte signature[8];
    std::fread(signature, 1, 8, fp);
    bool is_png = !png_sig_cmp(signature, 0, 8);
    if (!is_png)
    {
        std::fclose(fp);
        throw util::Exception("PNG signature did not match");
    }

    png_structp png = 0;
    png_infop png_info = 0;

    /* Initialize PNG structures. */
    png = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
        //(png_voidp)user_error_ptr, user_error_fn, user_warning_fn);
    if (!png)
    {
        std::fclose(fp);
        throw util::Exception("Out of memory");
    }

    png_info = png_create_info_struct(png);
    if (!png_info)
    {
        png_destroy_read_struct(&png, 0, 0);
        std::fclose(fp);
        throw util::Exception("Out of memory");
    }

    /* Init PNG file IO */
    png_init_io(png, fp);
    png_set_sig_bytes(png, 8);

    /* Read PNG header info. */
    png_read_info(png, png_info);

    /* Check if bit depth is valid. */
    int bit_depth = png_get_bit_depth(png, png_info);
    if (bit_depth > 8)
    {
        png_destroy_read_struct(&png, &png_info, 0);
        std::fclose(fp);
        throw util::Exception("PNG with more than 8 bit");
    }

    /* Apply transformations. */
    int color_type = png_get_color_type(png, png_info);
    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);
    if (png_get_valid(png, png_info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);

    /* Update the info struct to reflect the transformations. */
    png_read_update_info(png, png_info);

    /* Create image. */
    std::size_t width = png_get_image_width(png, png_info);
    std::size_t height = png_get_image_height(png, png_info);
    std::size_t channels = png_get_channels(png, png_info);
    ByteImage::Ptr image = ByteImage::create();
    image->allocate(width, height, channels);
    ByteImage::ImageData& data = image->get_data();

    /* Setup row pointers. */
    std::vector<png_bytep> row_pointers;
    row_pointers.resize(height);
    for (std::size_t i = 0; i < height; ++i)
        row_pointers[i] = &data[i * width * channels];

    /* Read the whole PNG in memory. */
    png_read_image(png, &row_pointers[0]);

    /* Clean up. */
    png_destroy_read_struct(&png, &png_info, 0);
    std::fclose(fp);

    return image;
}

/* ---------------------------------------------------------------- */

void
save_png_file (ByteImage::Ptr image, std::string const& filename)
{
    if (!image.get())
        throw std::invalid_argument("NULL image given");

    FILE *fp = std::fopen(filename.c_str(), "wb");
    if (!fp)
        throw util::FileException(filename, std::strerror(errno));

    //png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
    //    (png_voidp)user_error_ptr, user_error_fn, user_warning_fn);
    png_structp png_ptr = png_create_write_struct
        (PNG_LIBPNG_VER_STRING, 0, 0, 0);

    if (!png_ptr)
    {
        std::fclose(fp);
        throw util::Exception("Out of memory");
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
        std::fclose(fp);
        throw util::Exception("Out of memory");
    }

    png_init_io(png_ptr, fp);

    // void write_row_callback(png_ptr, png_uint_32 row, int pass);
    //png_set_write_status_fn(png_ptr, write_row_callback);

    /* Determine color type to be written. */
    int color_type;
    switch (image->channels())
    {
        case 1: color_type = PNG_COLOR_TYPE_GRAY; break;
        case 2: color_type = PNG_COLOR_TYPE_GRAY_ALPHA; break;
        case 3: color_type = PNG_COLOR_TYPE_RGB; break;
        case 4: color_type = PNG_COLOR_TYPE_RGB_ALPHA; break;
        default:
        {
            png_destroy_write_struct(&png_ptr, &info_ptr);
            std::fclose(fp);
            throw util::Exception("Cannot determine image color type");
        }
    }

    png_set_IHDR(png_ptr, info_ptr, image->width(), image->height(),
        8 /* Bit depth */, color_type, PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    /* Setup row pointers. */
    std::vector<png_bytep> row_pointers;
    row_pointers.resize(image->height());
    ByteImage::ImageData& data = image->get_data();
    for (std::size_t i = 0; i < row_pointers.size(); ++i)
        row_pointers[i] = &data[i * image->width() * image->channels()];

    /* Setup transformations. */
    int png_transforms = PNG_TRANSFORM_IDENTITY;
    //png_transforms |= PNG_TRANSFORM_INVERT_ALPHA;

    /* Write to file. */
    png_set_rows(png_ptr, info_ptr, &row_pointers[0]);
    png_write_png(png_ptr, info_ptr, png_transforms, NULL);
    png_write_end(png_ptr, info_ptr);

    /* Cleanup. */
    png_destroy_write_struct(&png_ptr, &info_ptr);
    std::fclose(fp);
}

#endif /* MVE_NO_PNG_SUPPORT */

/* ---------------------------------------------------------------- */

#ifndef MVE_NO_JPEG_SUPPORT

void
jpg_error_handler (j_common_ptr /*cinfo*/)
{
    throw util::Exception("JPEG format not recognized");
}

void
jpg_message_handler (j_common_ptr /*cinfo*/, int msg_level)
{
    if (msg_level < 0)
        throw util::Exception("JPEG data corrupt");
}

ByteImage::Ptr
load_jpg_file (std::string const& filename, std::string* exif)
{
    FILE* fp = std::fopen(filename.c_str(), "rb");
    if (fp == NULL)
        throw util::FileException(filename, std::strerror(errno));

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    ByteImage::Ptr image;

    try
    {
        /* Setup error handler and JPEG reader. */
        cinfo.err = jpeg_std_error(&jerr);
        jerr.error_exit = &jpg_error_handler;
        jerr.emit_message = &jpg_message_handler;
        jpeg_create_decompress(&cinfo);
        jpeg_stdio_src(&cinfo, fp);

        if (exif)
        {
            /* Request APP1 marker to be saved (this is the EXIF data). */
            jpeg_save_markers(&cinfo, JPEG_APP0+1, 0xffff);
        }

        /* Read JPEG header. */
        int ret = jpeg_read_header(&cinfo, false);
        if (ret != JPEG_HEADER_OK)
            throw util::Exception("JPEG header not recognized");

        /* Examine JPEG markers. */
        if (exif)
        {
            jpeg_saved_marker_ptr marker = cinfo.marker_list;
            //while (marker != 0) { ...; marker = marker->next; }
            if (marker)
            {
                char const* data = reinterpret_cast<char const*>(marker->data);
                exif->append(data, data + marker->data_length);
            }
        }

        /* Create image. */
        std::size_t width = cinfo.image_width;
        std::size_t height = cinfo.image_height;
        std::size_t channels = cinfo.jpeg_color_space;
        image = ByteImage::create(width, height, channels);
        ByteImage::ImageData& data = image->get_data();

        /* Start decompression. */
        jpeg_start_decompress(&cinfo);

        unsigned char* data_ptr = &data[0];
        while (cinfo.output_scanline < cinfo.output_height)
        {
            jpeg_read_scanlines(&cinfo, &data_ptr, 1);
            data_ptr += channels * cinfo.output_width;
        }

        /* Shutdown JPEG decompression. */
        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);
        std::fclose(fp);
    }
    catch (std::exception& /*e*/)
    {
        jpeg_destroy_decompress(&cinfo);
        std::fclose(fp);
        throw;
    }

    return image;
}

/* ---------------------------------------------------------------- */
// http://download.blender.org/source/chest/blender_2.03_tree/jpeg/example.c

void
save_jpg_file (ByteImage::Ptr image, std::string const& filename, int quality)
{
    if (!image.get())
        throw std::invalid_argument("NULL image given");

    FILE* fp = std::fopen(filename.c_str(), "wb");
    if (!fp)
        throw util::FileException(filename, std::strerror(errno));

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    /* Setup error handler and info object. */
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, fp);

    /* Specify image dimensions. */
    cinfo.image_width = image->width();
    cinfo.image_height = image->height();
    cinfo.input_components = image->channels();
    switch (image->channels())
    {
        case 1: cinfo.in_color_space = JCS_GRAYSCALE; break;
        case 3: cinfo.in_color_space = JCS_RGB; break;
        default:
        {
            jpeg_destroy_compress(&cinfo);
            std::fclose(fp);
            throw util::Exception("Cannot determine image color type");
        }
    }

    /* Set default compression parameters. */
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);
    jpeg_start_compress(&cinfo, TRUE);

    ByteImage::ImageData& data = image->get_data();
    int row_stride = image->width() * image->channels();
    while (cinfo.next_scanline < cinfo.image_height)
    {
        JSAMPROW row_pointer = &data[cinfo.next_scanline * row_stride];
        jpeg_write_scanlines(&cinfo, &row_pointer, 1);
    }
    jpeg_finish_compress(&cinfo);

    /* Cleanup. */
    jpeg_destroy_compress(&cinfo);
    std::fclose(fp);
}

#endif /* MVE_NO_JPEG_SUPPORT */

/* ---------------------------------------------------------------- */

#ifndef MVE_NO_TIFF_SUPPORT

void
tiff_error_handler (char const* /*module*/, char const* fmt, va_list ap)
{
    char msg[2048];
    ::vsprintf(msg, fmt, ap);
    throw util::Exception(msg);
}

ByteImage::Ptr
load_tiff_file (std::string const& filename)
{
    TIFFSetWarningHandler(0);
    TIFFSetErrorHandler(tiff_error_handler);

    TIFF* tif = TIFFOpen(filename.c_str(), "r");
    if (!tif)
        throw util::Exception("TIFF file format not recognized");

    try
    {
        /* Read width and height from TIFF and create MVE image. */
        uint32 width, height;
        uint16 channels, bits;
        TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
        TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
        TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &channels);
        TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bits);
        if (bits != 8)
            throw util::Exception("Expected 8 bit TIFF file");
        ByteImage::Ptr image = ByteImage::create(width, height, channels);

#if 1
        /* Scanline based TIFF reading. */
        uint32 rowstride = TIFFScanlineSize(tif);
        ByteImage::ImageData& data = image->get_data();
        for (uint32 row = 0; row < height; row++)
        {
            tdata_t row_pointer = &data[row * rowstride];
            TIFFReadScanline(tif, row_pointer, row);
        }
#endif

#if 0
        /* Simplest possible image reading. */
        ByteImage::ImageData& data = image->get_data();
        int ret = TIFFReadRGBAImageOriented(tif, width, height,
            (uint32_t*)&data[0], ORIENTATION_TOPLEFT, 0);
        if (ret == 0)
            throw util::Exception("Error reading TIFF image");

#endif

#if 0
        /* Simple image reading but in separate buffer first. */
        std::vector<uint32_t> buffer;
        buffer.resize(width * height);
        int ret = TIFFReadRGBAImageOriented(tif, width, height,
            &buffer[0], ORIENTATION_TOPLEFT, 0);

        if (ret == 0)
            throw util::Exception("Error reading TIFF image");

        for (std::size_t i = 0; i < buffer.size(); ++i)
        {
            image->at(i * 4 + 0) = TIFFGetR(buffer[i]);
            image->at(i * 4 + 1) = TIFFGetG(buffer[i]);
            image->at(i * 4 + 2) = TIFFGetB(buffer[i]);
            image->at(i * 4 + 3) = TIFFGetA(buffer[i]);
        }
        buffer.clear();
#endif

        TIFFClose(tif);
        return image;
    }
    catch (std::exception& e)
    {
        TIFFClose(tif);
        throw;
    }
}

/* ---------------------------------------------------------------- */

void
save_tiff_file (ByteImage::ConstPtr image, std::string const& filename)
{
    if (!image.get())
        throw std::invalid_argument("NULL image given");

    TIFF* tif = TIFFOpen(filename.c_str(), "w");
    if (!tif)
        throw util::FileException(filename, "Unknown TIFF file error");

    uint32_t width = image->width();
    uint32_t height = image->height();
    uint32_t channels = image->channels();
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, channels);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

    tdata_t buffer = const_cast<uint8_t*>(image->get_data_pointer());
    int ret = TIFFWriteEncodedStrip(tif, 0, buffer, image->get_value_amount());

    TIFFClose(tif);

    if (ret < 0)
        throw util::Exception("Error writing TIFF image");
}

/* ---------------------------------------------------------------- */

RawImage::Ptr
load_tiff_16_file (std::string const& filename)
{
    if (sizeof(uint16_t) != 2)
        throw util::Exception("Need 16bit data type for TIFF image.");

    TIFFSetWarningHandler(0);
    TIFFSetErrorHandler(tiff_error_handler);

    TIFF* tif = TIFFOpen(filename.c_str(), "r");
    if (!tif)
        throw util::Exception("TIFF file format not recognized");

    try
    {
        /* Read width and height from TIFF and create MVE image. */
        uint32 width, height;
        uint16 channels, bits;
        TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
        TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
        TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &channels);
        TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bits);
        if (bits != 16)
            throw util::Exception("TIFF file bits per sample don't match");

        RawImage::Ptr image = Image<uint16_t>::create(width, height, channels);

        /* Scanline based TIFF reading. */
        uint32 rowstride = TIFFScanlineSize(tif) / sizeof(uint16_t);
        Image<uint16_t>::ImageData& data = image->get_data();
        for (uint32 row = 0; row < height; row++)
        {
            tdata_t row_pointer = &data[row * rowstride];
            TIFFReadScanline(tif, row_pointer, row);
        }

        TIFFClose(tif);
        return image;
    }
    catch (std::exception& e)
    {
        TIFFClose(tif);
        throw;
    }
}

/* ---------------------------------------------------------------- */

void
save_tiff_16_file (RawImage::ConstPtr image, std::string const& filename)
{
    if (!image.get())
        throw std::invalid_argument("NULL image given");

    TIFF* tif = TIFFOpen(filename.c_str(), "w");
    if (!tif)
        throw util::FileException(filename, "Unknown TIFF file error");

    uint32_t width = image->width();
    uint32_t height = image->height();
    uint32_t channels = image->channels();
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, channels);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8 * sizeof(uint16_t));
    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

    tdata_t buffer = const_cast<uint16_t*>(image->get_data_pointer());
    int ret = TIFFWriteEncodedStrip(tif, 0, buffer,
        image->get_value_amount() * sizeof(uint16_t));

    TIFFClose(tif);

    if (ret < 0)
        throw util::Exception("Error writing TIFF image");
}

#endif /* MVE_NO_TIFF_SUPPORT */

/* ---------------------------------------------------------------- */

/*
 * PFM file format for float images.
 * http://netpbm.sourceforge.net/doc/pfm.html
 * FIXME: Convert to C++ I/O (std::fstream)
 */

FloatImage::Ptr
load_pfm_file (std::string const& filename)
{
    FILE *fp = std::fopen(filename.c_str(), "rb");
    if (!fp)
        throw util::FileException(filename, std::strerror(errno));

    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int channels = 0;

    char signature[2];
    std::fread(signature, 1, 2, fp);

    // check signature and determine channels
    if (signature[0] == 'P' && signature[1] == 'F')
        channels = 3;
    else if (signature[0] == 'P' && signature[1] == 'f')
        channels = 1;
    else
    {
        std::fclose(fp);
        throw util::Exception("PFM signature did not match");
    }

    // read width and height
    std::fscanf(fp, "%u %u", &width, &height);
    // read scale
    float scale = 0.0f;
    std::fscanf(fp, "%f\n", &scale);

    // create image
    FloatImage::Ptr image = FloatImage::create();
    image->allocate(width, height, channels);

    // fill image with data from file
    std::fread(image->get_byte_pointer(), 1, image->get_byte_size(), fp);
    std::fclose(fp);

    // TODO: scale handling
    if (scale < 0.0f)
    {
        std::transform(image->get_data().begin(), image->get_data().end(),
        image->get_data().begin(),
        (float(*)(float const&))util::system::letoh<float>);
        scale *= -1.0f;
    }
    else
    {
        std::transform(image->get_data().begin(), image->get_data().end(),
        image->get_data().begin(),
        (float(*)(float const&))util::system::betoh<float>);
    }

    return image;
}

/* ---------------------------------------------------------------- */

void
save_pfm_file (FloatImage::ConstPtr image, std::string const& filename)
{
    if (!image.get())
        throw std::invalid_argument("NULL image given");

    FILE *fp = std::fopen(filename.c_str(), "wb");
    if (!fp)
        throw util::FileException(filename, std::strerror(errno));

    /* Determine color type to be written. */
    switch (image->channels())
    {
        case 1: std::fprintf(fp, "Pf\n"); break;
        case 3: std::fprintf(fp, "PF\n"); break;
        default:
        {
            std::fclose(fp);
            throw util::Exception("Can only handle 1 or 3 channel images");
        }
    }

    std::fprintf(fp, "%u %u\n",
        (unsigned int)image->width(), (unsigned int)image->height());
#ifdef HOST_BYTEORDER_LE
    std::fprintf(fp, "-1.000000\n"); // we currently don't support scales
#else
    std::fprintf(fp, "1.000000\n"); // we currently don't support scales
#endif
    std::fwrite(image->get_byte_pointer(), 1, image->get_byte_size(), fp);
    std::fclose(fp);
}

/* ---------------------------------------------------------------- */

/*
 * PPM file format for 8 and 16 bit images. The bit8 argument is set to
 * true, 8 bit PPMs with maxval set to less than 256 are loaded.
 * Otherwise, 16 bit PPMs with maxval less than 65536 are loaded.
 * http://netpbm.sourceforge.net/doc/ppm.html
 */
ImageBase::Ptr
load_ppm_file_intern (std::string const& filename, bool bit8)
{
    std::ifstream in(filename.c_str());
    if (!in.good())
        throw util::FileException(filename, std::strerror(errno));

    int width = 0;
    int height = 0;
    int channels = 0;
    int maxval = 0;

    char signature[2];
    in.read(signature, 2);

    // check signature and determine channels
    if (signature[0] == 'P' && signature[1] == '5')
        channels = 1;
    else if (signature[0] == 'P' && signature[1] == '6')
        channels = 3;
    else
    {
        in.close();
        throw util::Exception("PPM signature did not match");
    }

    /* Read width and height as well as max value. */
    in >> width >> height >> maxval;

    /* Read final whitespace character. */
    char temp;
    in.read(&temp, 1);

    /* Check image width and height. Shouldn't be too large. */
    if (width * height > 268435356)
    {
        in.close();
        throw util::Exception("Image too friggin huge");
    }

    ImageBase::Ptr ret;

    /* Check max value. Should be in [256, 65535]. */
    if (maxval < 256 && bit8)
    {
        /* Read image content. */
        ByteImage::Ptr image = ByteImage::create(width, height, channels);
        in.read(image->get_byte_pointer(), image->get_byte_size());
        ret = image;
    }
    else if (maxval < 65536 && !bit8)
    {
        /* Read image content. */
        RawImage::Ptr image = RawImage::create(width, height, channels);
        in.read(image->get_byte_pointer(), image->get_byte_size());
        ret = image;
    }
    else
    {
        in.close();
        throw util::Exception("PPM max value is invalid");
    }

    in.close();
    return ret;
}

/* ---------------------------------------------------------------- */

RawImage::Ptr
load_ppm_16_file (std::string const& filename)
{
    return load_ppm_file_intern(filename, false);
}

ByteImage::Ptr
load_ppm_file (std::string const& filename)
{
    return load_ppm_file_intern(filename, true);
}

/* ---------------------------------------------------------------- */

void
save_ppm_file_intern (ImageBase::ConstPtr image, std::string const& filename)
{
    if (!image.get())
        throw std::invalid_argument("NULL image given");

    if (image->channels() != 1 && image->channels() != 3)
        throw std::invalid_argument("Supports 1 and 3 channel images only");

    std::ofstream out(filename.c_str());
    if (!out.good())
        throw util::FileException(filename, std::strerror(errno));

    if (image->channels() == 1)
        out << "P5\n";
    else if (image->channels() == 3)
        out << "P6\n";
    else
        throw std::runtime_error("Supports 1 and 3 channel images only");

    int maxval = 0;
    if (image->get_type() == IMAGE_TYPE_UINT8)
        maxval = 255;
    else if (image->get_type() == IMAGE_TYPE_UINT16)
        maxval = 65535;
    else
    {
        out.close();
        throw util::Exception("Invalid image format");
    }

    out << image->width() << " " << image->height() << " " << maxval << "\n";
    out.write(image->get_byte_pointer(), image->get_byte_size());
    out.close();
}

/* ---------------------------------------------------------------- */

void
save_ppm_16_file (RawImage::ConstPtr image, std::string const& filename)
{
    save_ppm_file_intern(image, filename);
}

void
save_ppm_file (ByteImage::ConstPtr image, std::string const& filename)
{
    save_ppm_file_intern(image, filename);
}

MVE_IMAGE_NAMESPACE_END
MVE_NAMESPACE_END

