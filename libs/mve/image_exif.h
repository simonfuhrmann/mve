/*
 * Tiny EXIF tag extractor.
 * Interface written by Simon Fuhrmann.
 * Original code written by Mayank Lahiri (see .cc file).
 *
 * Original code from: http://code.google.com/p/easyexif/
 * Some docs here: http://www.awaresystems.be/imaging/tiff/tifftags/privateifd/exif.html
 * More docs: http://www.sno.phy.queensu.ca/~phil/exiftool/TagNames/EXIF.html
 * Even more docs: http://paulbourke.net/dataformats/tiff/
 */

#ifndef MVE_IMAGEEXIF_HEADER
#define MVE_IMAGEEXIF_HEADER

#include <iostream>
#include <string>

#include "mve/defines.h"

MVE_NAMESPACE_BEGIN
MVE_IMAGE_NAMESPACE_BEGIN

/**
 * EXIF information.
 */
struct ExifInfo
{
    std::string camera_model; ///< camera model
    std::string camera_maker; ///< camera manufacturer
    std::string date_modified; ///< date/time string of last modification
    std::string date_original; ///< date/time string of original image
    std::string description; ///< description of the image
    int iso_speed; ///< Cameras ISO speed ratings
    float focal_length; ///< focal length in mm
    float focal_length_35mm; ///< focal length equivalent for 35mm film
    float f_number; ///< f-number in 1/f
    float exposure_time; ///< in seconds
};

/**
 * Function to extract a few EXIF tags from binary data.
 * The function accepts pure EXIF binary data as read from the
 * JPEG file, or the complete JPEG file. In the latter case,
 * argument 'is_jpeg' needs to be true.
 */
ExifInfo
exif_extract (char const* data, unsigned int len, bool is_jpeg = false);

/**
 * Prints the EXIF information to stdout.
 */
void
exif_debug_print (std::ostream& stream, ExifInfo const& exif);

MVE_IMAGE_NAMESPACE_END
MVE_NAMESPACE_END

#endif /* MVE_IMAGEEXIF_HEADER */
