/*
 * Copyright (C) 2015, Benjamin Richter, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef DMRECON_IMAGE_PYRAMID_H
#define DMRECON_IMAGE_PYRAMID_H

#include <vector>
#include <memory>
#include <map>
#include <mutex>

#include "mve/scene.h"
#include "mve/view.h"
#include "mve/image_base.h"

#include "mve/camera.h"
#include "math/matrix.h"
#include "dmrecon/defines.h"

MVS_NAMESPACE_BEGIN

struct ImagePyramidLevel
{
    int width, height;
    mve::ByteImage::ConstPtr image;
    math::Matrix3f proj;
    math::Matrix3f invproj;

    ImagePyramidLevel();
    ImagePyramidLevel(mve::CameraInfo const& _cam, int width, int height);
};

inline
ImagePyramidLevel::ImagePyramidLevel()
    : width(0)
    , height(0)
{
}

inline
ImagePyramidLevel::ImagePyramidLevel(mve::CameraInfo const& cam,
    int _width, int _height)
    : width(_width)
    , height(_height)
{
    cam.fill_calibration(*proj, width, height);
    cam.fill_inverse_calibration(*invproj, width, height);
}

/**
  * Image pyramids are represented as vectors of pyramid levels,
  * where the presence of an image in a specific level indicates
  * that all levels with higher indices also contain images.
  */
class ImagePyramid : public std::vector<ImagePyramidLevel>
{
public:
    typedef std::shared_ptr<ImagePyramid> Ptr;
    typedef std::shared_ptr<ImagePyramid const> ConstPtr;
};

class ImagePyramidCache
{
public:
    static ImagePyramid::ConstPtr get(mve::Scene::Ptr scene,
        mve::View::Ptr view, std::string embeddingName, int minLevel);
    static void cleanup();

private:
    static std::mutex metadataMutex;
    static mve::Scene::Ptr cachedScene;
    static std::string cachedEmbedding;

    static std::map<int, ImagePyramid::Ptr> entries;
};

MVS_NAMESPACE_END

#endif
