/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef MVE_BUNDLE_IO_HEADER
#define MVE_BUNDLE_IO_HEADER

#include <string>

#include "mve/bundle.h"
#include "mve/defines.h"

MVE_NAMESPACE_BEGIN

/* ------------------- MVE native bundle format ------------------- */

/** TODO: For now refers to load_photosynther_bundle(). */
Bundle::Ptr
load_mve_bundle (std::string const& filename);

/** TODO: For now refers to save_photosynther_bundle(). */
void
save_mve_bundle (Bundle::ConstPtr bundle, std::string const& filename);

/* -------------- Support for NVM files (VisualSFM) --------------- */

/**
 * Per-camera NVM specific information.
 */
struct NVMCameraInfo
{
    /** Path the the original image file. */
    std::string filename;
    /** The single radial distortion parameter. */
    float radial_distortion;
};

/**
 * Loads an NVM bundle file while providing NVM specific information.
 * Docs: http://homes.cs.washington.edu/~ccwu/vsfm/doc.html#nvm
 *
 * This function provides a bundle with cameras where the focal length is in
 * VisualSFM conventions, NOT MVE conventions. To convert to focal length to
 * MVE conventions, it must be divided by the maximum image dimension.
 */
Bundle::Ptr
load_nvm_bundle (std::string const& filename,
    std::vector<NVMCameraInfo>* camera_info = nullptr);

/* ------------------ Support for Noah's Bundler  ----------------- */

/**
 * Loads a Bundler bundle file.
 * The parser does not provide Bundler specific information.
 *
 * This function provides a bundle where the 2D floating point keypoint
 * positions are given in bundlers image-centered coordinate system.
 */
Bundle::Ptr
load_bundler_bundle (std::string const& filename);

/* ------------------- Support for Photosynther ------------------- */

/**
 * Loads a Photosynther bundle file.
 * The parser does not provide Photosynther specific information.
 */
Bundle::Ptr
load_photosynther_bundle (std::string const& filename);

/**
 * Writes a Photosynther bundle file.
 */
void
save_photosynther_bundle (Bundle::ConstPtr bundle,
    std::string const& filename);

MVE_NAMESPACE_END

#endif /* MVE_BUNDLE_IO_HEADER */
