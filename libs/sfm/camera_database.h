/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef SFM_CAMERA_DATABASE_HEADER
#define SFM_CAMERA_DATABASE_HEADER

#include <vector>
#include <string>

#include "sfm/defines.h"

SFM_NAMESPACE_BEGIN

/**
 * Representation of a digital camera.
 */
struct CameraModel
{
    /** The manufacturer for the camera. */
    std::string maker;
    /** The model of the camera. */
    std::string model;
    /** The width of the sensor in milli meters. */
    float sensor_width_mm;
    /** The height of the sensor in milli meters. */
    float sensor_height_mm;
    /** The width of the sensor in pixels. */
    int sensor_width_px;
    /** The height of the sensor in pixels. */
    int sensor_height_px;
};

/* ---------------------------------------------------------------- */

/**
 * Camera database which, given a maker and model string, will look for
 * a camera model in the database and return the model on successful lookup.
 * If the lookup fails, a null pointer is returned.
 */
class CameraDatabase
{
public:
    /** Access to the singleton object. */
    static CameraDatabase* get (void);

    /** Lookup of a camera model. Returns null on failure. */
    CameraModel const* lookup (std::string const& maker,
        std::string const& model) const;

private:
    CameraDatabase (void);
    void add (std::string const& maker, std::string const& model,
        float sensor_width_mm, float sensor_height_mm,
        int sensor_width_px, int sensor_height_px);

private:
    static CameraDatabase* instance;
    std::vector<CameraModel> data;
};

/* ------------------------ Implementation ------------------------ */

inline CameraDatabase*
CameraDatabase::get (void)
{
    if (CameraDatabase::instance == nullptr)
        CameraDatabase::instance = new CameraDatabase();
    return CameraDatabase::instance;
}

SFM_NAMESPACE_END

#endif /* SFM_CAMERA_DATABASE_HEADER */
