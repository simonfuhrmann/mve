/*
 * Image tools: loading and saving.
 * Written by Simon Fuhrmann, Jens Ackermann, Sebastian Koch.
 */

#ifndef MVE_IMAGE_FILE_HEADER
#define MVE_IMAGE_FILE_HEADER

#include <string>

#include "mve/defines.h"
#include "mve/image.h"

MVE_NAMESPACE_BEGIN
MVE_IMAGE_NAMESPACE_BEGIN

/*
 * ------------------ Image loading and saving --------------------
 *
 * libpng docs: http://www.libpng.org/pub/png/libpng-manual.txt
 * libjpg docs: http://apodeline.free.fr/DOC/libjpeg/libjpeg.html
 * libtiff docs: http://www.libtiff.org/libtiff.html
 * pfm file docs: http://netpbm.sourceforge.net/doc/pfm.html
 * ppm file docs: http://en.wikipedia.org/wiki/Netpbm_format
 * more ppm file docs: http://netpbm.sourceforge.net/doc/#formats
 *
 * TODO
 * - Throw-safe read/write streams (not important)
 * - Read and display PNG embedded "keywords" (author, comment, etc.)
 * - Fix PPM endianess
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
 * PNG has 1, 2, 3 or 4 channels with gray, gray-alpha, RGB or RGBA values.
 * Conversion of 1, 2, 4 and 16 bit values to 8 bit is auto-applied.
 * May throw util::FileException and util::Exception.
 */
ByteImage::Ptr
load_png_file (std::string const& filename);

/**
 * Saves image data to a PNG file. Supports 1, 2, 3 and 4 channel images.
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
load_jpg_file (std::string const& filename, std::string* exif = NULL);

/**
 * Saves image data to a JPG file. Supports 1 and 3 channel images.
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
 * Writes a TIFF to file. Supports any number of channels.
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
 * Writes a 16bit TIFF to file. Supports any number of channels.
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
 * Saves float image data to PFM file. Supports 1 and 3 channel images.
 * May throw util::FileException and util::Exception.
 */
void
save_pfm_file (FloatImage::ConstPtr image, std::string const& filename);

/* ------------------------- PPM16 support ------------------------- */

/**
 * Loads a 16 bit PPM file.
 * May throw util::FileException and util::Exception.
 */
RawImage::Ptr
load_ppm_16_file (std::string const& filename);

/**
 * Save a 16 bit PPM file. Supports 1 and 3 channel images.
 * May throw util::FileException and util::Exception.
 */
void
save_ppm_16_file (RawImage::ConstPtr image, std::string const& filename);

/* ------------------------- PPM support ------------------------- */

/**
 * Loads a 8 bit PPM file.
 * May throw util::FileException and util::Exception.
 */
ByteImage::Ptr
load_ppm_file (std::string const& filename);

/**
 * Writes a 8 bit PPM file. Supports 1 and 3 channel images.
 * May throw util::FileException and std::invalid_argument.
 */
void
save_ppm_file (ByteImage::ConstPtr image, std::string const& filename);

MVE_IMAGE_NAMESPACE_END
MVE_NAMESPACE_END

#endif /* MVE_IMAGE_FILE_HEADER */
