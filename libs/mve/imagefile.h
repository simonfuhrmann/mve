/*
 * Image tools: loading and saving
 * Written by Simon Fuhrmann, Jens Ackermann, Sebastian Koch.
 */

#ifndef MVE_IMAGE_FILE_HEADER
#define MVE_IMAGE_FILE_HEADER

#include <string>

#include "defines.h"
#include "image.h"

#ifdef _WIN32
#   define MVE_NO_TIFF_SUPPORT
#endif

MVE_NAMESPACE_BEGIN
MVE_IMAGE_NAMESPACE_BEGIN

/*
 * ------------------ Image loading and saving --------------------
 *
 * libpng docs: http://www.libpng.org/pub/png/libpng-manual.txt
 * libjpg docs: http://www.jpegcameras.com/libjpeg/libjpeg-2.html
 * libtiff docs: http://www.libtiff.org/libtiff.html
 * pfm file docs: http://netpbm.sourceforge.net/doc/pfm.html
 *
 * TODO
 * - Throw-safe read/write streams (not important)
 * - Read and display PNG embedded "keywords" (author, comment, etc.)
 */

/**
 * Loads an image, detecting file type.
 * May throw util::Exception.
 */
ByteImage::Ptr
load_file (std::string const& filename);

/**
 * Saves a byte image to file, detecting file type.
 * May throw util::Exception.
 */
void
save_file (ByteImage::Ptr image, std::string const& filename);

/**
 * Saves a float image to file, detecting file type.
 * May throw util::Exception.
 */
void
save_file (FloatImage::Ptr image, std::string const& filename);

/* -------------------------- PNG support ------------------------- */

#ifndef MVE_NO_PNG_SUPPORT

/**
 * Loads a PNG file.
 * Conversion of 1, 2, 4 and 16 bit values to 8 bit is auto-applied.
 * PNG has 1 to 4 channels with gray, gray-alpha, RGB or RGBA values.
 * May throw util::FileException and util::Exception.
 */
ByteImage::Ptr
load_png_file (std::string const& filename);

/**
 * Saves image data to a PNG file.
 * May throw util::FileException and util::Exception.
 */
void
save_png_file (ByteImage::Ptr image, std::string const& filename);

#endif /* MVE_NO_PNG_SUPPORT */

/* ------------------------- JPEG support ------------------------- */

#ifndef MVE_NO_JPEG_SUPPORT

/**
 * Loads a JPG file.
 * JPGs have 1 or 3 channels, a gray-value channel or RGB channels.
 * Optional EXIF data may be loaded into string pointed to by 'exif'.
 * May throw util::FileException and util::Exception.
 */
ByteImage::Ptr
load_jpg_file (std::string const& filename, std::string* exif = 0);

/**
 * Saves image data to a JPG file.
 * The quality value is in range [0, 100] from worst to best quality.
 * May throw util::FileException and util::Exception.
 */
void
save_jpg_file (ByteImage::Ptr image, std::string const& filename, int quality);

#endif /* MVE_NO_JPEG_SUPPORT */

/* ------------------------- TIFF support ------------------------- */

#ifndef MVE_NO_TIFF_SUPPORT

/**
 * Loads a TIFF file.
 * May throw util::FileException and util::Exception.
 */
ByteImage::Ptr
load_tiff_file (std::string const& filename);

/**
 * Writes a TIFF to file.
 * May throw util::FileException and util::Exception.
 */
void
save_tiff_file (ByteImage::ConstPtr image, std::string const& filename);

/**
 * Loads a 16bit TIFF file.
 * May throw util::FileException and util::Exception.
 */
RawImage::Ptr
load_tiff_16_file (std::string const& filename);

/**
 * Writes a 16bit TIFF to file.
 * May throw util::FileException and util::Exception.
 */
void
save_tiff_16_file (RawImage::ConstPtr image, std::string const& filename);

#endif /* MVE_NO_TIFF_SUPPORT */

/* -------------------------- PFM support ------------------------- */

/**
 * Loads a PFM file.
 * Only handles 1 or 3 channel images of float values. No scaling is
 * performed. May throw util::FileException and util::Exception.
 */
FloatImage::Ptr
load_pfm_file (std::string const& filename);

/**
 * Saves float image data to PFM file.
 * May throw util::FileException and util::Exception.
 */
void
save_pfm_file (FloatImage::ConstPtr image, std::string const& filename);

/* ------------------------- PPM16 support ------------------------- */

/**
 * Loads a 16 bit PPM file.
 * May throw util::FileException and util::Exception.
 */
FloatImage::Ptr
load_ppm_16_file (std::string const& filename);

/**
 * Save a 16 bit PPM file.
 * May throw util::FileException and util::Exception.
 */
void
save_ppm_16_file (FloatImage::ConstPtr image, std::string const& filename);


MVE_IMAGE_NAMESPACE_END
MVE_NAMESPACE_END

#endif /* MVE_IMAGE_FILE_HEADER */
