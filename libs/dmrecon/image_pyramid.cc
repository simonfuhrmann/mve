/*
 * Copyright (C) 2015, Benjamin Richter, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include "dmrecon/image_pyramid.h"

#include "mve/image_tools.h"
#include <cassert>

MVS_NAMESPACE_BEGIN

namespace
{
    int const MIN_IMAGE_DIM = 30;

    ImagePyramid::Ptr
    buildPyramid(mve::View::Ptr view, std::string embeddingName)
    {
        ImagePyramid::Ptr pyramid(new ImagePyramid());
        ImagePyramid& levels = *pyramid;

        mve::View::ImageProxy const* proxy = view->get_image_proxy(embeddingName);
        mve::CameraInfo cam = view->get_camera();

        assert(proxy != nullptr);

        int curr_width = proxy->width;
        int curr_height = proxy->height;

        levels.push_back(ImagePyramidLevel(cam, curr_width, curr_height));

        while (std::min(curr_width, curr_height) >= MIN_IMAGE_DIM)
        {
            if (curr_width % 2 == 1)
                cam.ppoint[0] = cam.ppoint[0] * float(curr_width)
                        / float(curr_width + 1);
            if (curr_height % 2 == 1)
                cam.ppoint[1] = cam.ppoint[1] * float(curr_height)
                        / float(curr_height + 1);

            curr_width = (curr_width + 1) / 2;
            curr_height = (curr_height + 1) / 2;

            levels.push_back(ImagePyramidLevel(cam, curr_width, curr_height));
        }

        return pyramid;
    }

    void
    ensureImages(ImagePyramid& levels, mve::View::Ptr view,
        std::string embeddingName, int minLevel)
    {
        if (levels[minLevel].image != nullptr)
            return;

        mve::ByteImage::Ptr img = view->get_byte_image(embeddingName);
        int channels = img->channels();

        /* Remove alpha channel. */
        if (channels == 2 || channels == 4)
            mve::image::reduce_alpha<uint8_t>(img);
        /* Expand grayscale images to RGB. */
        if (img->channels() == 1)
            img = mve::image::expand_grayscale<uint8_t>(img);
        /* Expect 3-channel images. */
        if (img->channels() != 3)
            throw std::invalid_argument("Image with invalid number of channels");

        /* Create image pyramid. */
        int curr_width = img->width();
        int curr_height = img->height();
        for (int i = 0; std::min(curr_width, curr_height) >= MIN_IMAGE_DIM; ++i)
        {
            if (levels[i].image != nullptr)
                break;

            if (i > 0)
            {
                img = mve::image::rescale_half_size_gaussian<uint8_t>(img, 1.f);
                curr_width = img->width();
                curr_height = img->height();
            }

            if (minLevel <= i)
                levels[i].image = img;
        }

        view->cache_cleanup();
    }
}

ImagePyramid::ConstPtr
ImagePyramidCache::get(mve::Scene::Ptr scene, mve::View::Ptr view,
    std::string embeddingName, int minLevel)
{
    std::lock_guard<std::mutex> lock(ImagePyramidCache::metadataMutex);

    /* Initialize on first access. */
    if (ImagePyramidCache::cachedScene == nullptr)
    {
        ImagePyramidCache::cachedScene = scene;
        ImagePyramidCache::cachedEmbedding = embeddingName;
    }

    ImagePyramid::Ptr pyramid;

    if (scene != ImagePyramidCache::cachedScene
        || embeddingName != ImagePyramidCache::cachedEmbedding)
    {
        /* create own pyramid because shared pyramid is incompatible. */
        pyramid = buildPyramid(view, embeddingName);
    }
    else
    {
        /* Either re-recreate or use cached entry. */
        pyramid = ImagePyramidCache::entries[view->get_id()];
        if (pyramid == nullptr)
        {
            pyramid = buildPyramid(view, embeddingName);
            ImagePyramidCache::entries[view->get_id()] = pyramid;
        }
    }

    ensureImages(*pyramid, view, embeddingName, minLevel);
    return pyramid;
}

void
ImagePyramidCache::cleanup()
{
    std::lock_guard<std::mutex> lock(ImagePyramidCache::metadataMutex);

    if (ImagePyramidCache::cachedScene == nullptr)
        return;

    for (std::map<int, ImagePyramid::Ptr>::iterator it = ImagePyramidCache::entries.begin();
         it != ImagePyramidCache::entries.end(); ++it)
    {
        if (it->second == nullptr)
            continue;

        if (it->second.use_count() == 1)
        {
            it->second.reset();
            ImagePyramidCache::cachedScene->get_view_by_id(it->first)->cache_cleanup();
        }
    }
}

/* static fields of ImgPyramidCache: */
std::mutex ImagePyramidCache::metadataMutex;
mve::Scene::Ptr ImagePyramidCache::cachedScene;
std::string ImagePyramidCache::cachedEmbedding = "";
std::map<int, ImagePyramid::Ptr> ImagePyramidCache::entries;

MVS_NAMESPACE_END
