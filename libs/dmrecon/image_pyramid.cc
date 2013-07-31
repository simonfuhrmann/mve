#include "dmrecon/image_pyramid.h"

#include "mve/imagetools.h"

MVS_NAMESPACE_BEGIN

namespace
{
    int const MIN_IMAGE_DIM = 30;

    ImagePyramid::Ptr
    buildPyramid(mve::View::Ptr view, std::string embeddingName)
    {
        ImagePyramid::Ptr pyramid(new ImagePyramid());
        ImagePyramid& levels = *pyramid;

        mve::MVEFileProxy const* proxy = view->get_proxy(embeddingName);
        mve::CameraInfo cam = view->get_camera();

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
        if (levels[minLevel].image != NULL)
            return;

        mve::ImageBase::Ptr img = view->get_image(embeddingName);
        int channels = img->channels();
        mve::ImageType type = img->get_type();

        switch (type)
        {
            case mve::IMAGE_TYPE_UINT8:
                if (channels == 2 || channels == 4)
                    mve::image::reduce_alpha<uint8_t>(img);
                if (img->channels() == 1)
                    img = mve::image::expand_grayscale<uint8_t>(img);
                break;

            case mve::IMAGE_TYPE_FLOAT:
                if (channels == 2 || channels == 4)
                    mve::image::reduce_alpha<float>(img);
                if (img->channels() == 1)
                    img = mve::image::expand_grayscale<float>(img);
                break;

            default:
                throw util::Exception("Invalid image type");
        }

        if (img->channels() != 3)
            throw std::invalid_argument("Image with invalid number of channels");

        /* create image pyramid */
        int curr_width = img->width();
        int curr_height = img->height();

        int i = 0;

        if (0 >= minLevel)
            levels[0].image = img;

        while (std::min(curr_width, curr_height) >= MIN_IMAGE_DIM)
        {
            ++i;

            if (levels[i].image != NULL)
                break;

            if (type == mve::IMAGE_TYPE_UINT8)
                img = mve::image::rescale_half_size_gaussian<uint8_t>(img, 1.f);
            else if (type == mve::IMAGE_TYPE_FLOAT)
                img = mve::image::rescale_half_size_gaussian<float>(img, 1.f);
            else
                throw util::Exception("Invalid image type");

            /* compute new projection matrix */
            curr_width = img->width();
            curr_height = img->height();

            if (i >= minLevel)
                levels[i].image = img;
        }

        view->cache_cleanup();
    }

}

ImagePyramid::ConstPtr
ImagePyramidCache::get(mve::Scene::Ptr scene, mve::View::Ptr view,
                     std::string embeddingName, int minLevel)
{
    util::MutexLock lock(ImagePyramidCache::metadataMutex);

    /* initialize on first access */
    if (ImagePyramidCache::cachedScene == NULL)
    {
        ImagePyramidCache::cachedScene = scene;
        ImagePyramidCache::cachedEmbedding = embeddingName;
    }

    ImagePyramid::Ptr pyramid;

    if (scene != ImagePyramidCache::cachedScene
        || embeddingName != ImagePyramidCache::cachedEmbedding)
    {
        pyramid = buildPyramid(view, embeddingName);
    }
    else
    {
        pyramid = ImagePyramidCache::entries[view->get_id()];
        if (pyramid == NULL)
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
    util::MutexLock lock(ImagePyramidCache::metadataMutex);

    if (ImagePyramidCache::cachedScene == NULL)
        return;

    for (std::map<int, ImagePyramid::Ptr>::iterator it = ImagePyramidCache::entries.begin();
         it != ImagePyramidCache::entries.end(); ++it)
    {
        if (it->second == NULL)
            continue;

        if (it->second.use_count() == 1)
        {
            it->second.reset();
            ImagePyramidCache::cachedScene->get_view_by_id(it->first)->cache_cleanup();
        }
    }
}

/* static fields of ImgPyramidCache: */
util::Mutex ImagePyramidCache::metadataMutex;
mve::Scene::Ptr ImagePyramidCache::cachedScene;
std::string ImagePyramidCache::cachedEmbedding = "";
std::map<int, ImagePyramid::Ptr> ImagePyramidCache::entries;

MVS_NAMESPACE_END
