/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef SFM_EXTRACT_FOCAL_LENGTH_HEADER
#define SFM_EXTRACT_FOCAL_LENGTH_HEADER

#include <utility>

#include "mve/image_exif.h"
#include "sfm/defines.h"

SFM_NAMESPACE_BEGIN

/**
 * Indicator which focal length estimation has been used.
 */
enum FocalLengthMethod
{
    FOCAL_LENGTH_AND_DATABASE,
    FOCAL_LENGTH_35MM_EQUIV,
    FOCAL_LENGTH_FALLBACK_VALUE
};

/**
 * Datatype for the focal length estimate which reports the normalized
 * focal length as well as the method used to obtain the value.
 */
typedef std::pair<float, FocalLengthMethod> FocalLengthEstimate;

/**
 * Extracts the focal length from the EXIF tags of an image.
 *
 * The algorithm first checks for the availability of the "focal length"
 * in EXIF tags and computes the effective focal length using a database
 * of camera sensor sizes. If the camera model is unknown to the database,
 * the "focal length 35mm equivalent" EXIF tag is used. If this information
 * is also not available, a default value is used.
 *
 * This estimation can fail in numerous situations:
 *  - The image contains no EXIF tags (default value is used)
 *  - The camera did not specify the focal length in EXIF
 *  - The lens specifies the wrong focal length due to lens incompatibility
 *  - The camera is not in the database and the 35mm equivalent is missing
 *  - The camera used digial zoom changing the effective focal length
 *
 * The resulting focal length is in normalized format, that is the quotient
 * of the image focal length by the sensor size. E.g. a photo taken at 70mm
 * with a 35mm sensor size will result in a normalized focal length of 2.
 */
FocalLengthEstimate
extract_focal_length (mve::image::ExifInfo const& exif);

SFM_NAMESPACE_END

#endif /* SFM_EXTRACT_FOCAL_LENGTH_HEADER */
