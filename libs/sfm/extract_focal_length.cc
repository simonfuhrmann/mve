/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <vector>
#include <utility>

#include "sfm/camera_database.h"
#include "sfm/extract_focal_length.h"

SFM_NAMESPACE_BEGIN

std::pair<float, FocalLengthMethod>
extract_focal_length (mve::image::ExifInfo const& exif)
{
    /* Step 1: Check for focal length info in EXIF and database entry. */
    float focal_length = exif.focal_length;
    std::string camera_maker = exif.camera_maker;
    std::string camera_model = exif.camera_model;
    float sensor_size = -1.0f;
    if (focal_length > 0.0f && !camera_model.empty())
    {
        CameraDatabase const* db = CameraDatabase::get();
        CameraModel const* model = db->lookup(camera_maker, camera_model);
        if (model != nullptr)
            sensor_size = model->sensor_width_mm;
    }
    if (focal_length > 0.0f && sensor_size > 0.0f)
    {
        float flen = focal_length / sensor_size;
        return std::make_pair(flen, FOCAL_LENGTH_AND_DATABASE);
    }

    /* Step 2: Check for 35mm equivalent focal length. */
    float focal_length_35mm = exif.focal_length_35mm;
    if (focal_length_35mm > 0.0f)
    {
        float flen = focal_length_35mm / 35.0f;
        return std::make_pair(flen, FOCAL_LENGTH_35MM_EQUIV);
    }

    /* Step 3: Fall back to default value. */
    return std::make_pair(1.0f, FOCAL_LENGTH_FALLBACK_VALUE);
}

SFM_NAMESPACE_END
