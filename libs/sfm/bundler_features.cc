#include "mve/image_exif.h"
#include "mve/image_tools.h"
#include "sfm/bundler_common.h"
#include "sfm/extract_focal_length.h"
#include "sfm/bundler_features.h"

SFM_NAMESPACE_BEGIN
SFM_BUNDLER_NAMESPACE_BEGIN

void
Features::compute (mve::Scene::Ptr scene, ViewportList* viewports)
{
    if (scene == NULL)
        throw std::invalid_argument("NULL scene given");

    if (viewports == NULL)
        throw std::invalid_argument("No viewports given");

    mve::Scene::ViewList const& views = scene->get_views();

    /* Initialize viewports. */
    if (viewports != NULL)
    {
        viewports->clear();
        viewports->resize(views.size());
    }

    /* Iterate the scene and compute features. */
#pragma omp parallel for schedule(dynamic,1)
    for (std::size_t i = 0; i < views.size(); ++i)
    {
        if (views[i] == NULL)
            continue;

        Viewport* viewport = (viewports == NULL ? NULL : &viewports->at(i));
        mve::View::Ptr view = views[i];

        mve::ByteImage::Ptr image
            = view->get_byte_image(this->opts.image_embedding);
        if (image == NULL)
            continue;

        /* Rescale image until maximum image size is met. */
        while (image->width() * image->height() > this->opts.max_image_size)
            image = mve::image::rescale_half_size<uint8_t>(image);

#pragma omp critical
        std::cout << "Computing features for view ID " << view->get_id()
            << " (" << image->width() << "x" << image->height()
            << ")..." << std::endl;

        /* Compute features for view. */
        viewport->features.set_options(this->opts.feature_options);
        viewport->features.compute_features(image);
        viewport->width = image->width();
        viewport->height = image->height();

        /* Try to get an estimate for the focal length. */
        this->estimate_focal_length(view, viewport);

        /* Clean up unused embeddings. */
        image.reset();
        view->cache_cleanup();
    }
}

void
Features::estimate_focal_length (mve::View::Ptr view, Viewport* viewport) const
{
    if (viewport == NULL)
        return;

    /* Try to get EXIF data. */
    if (this->opts.exif_embedding.empty())
    {
        this->fallback_focal_length(view, viewport);
        return;
    }
    mve::ByteImage::Ptr exif_data = view->get_data(this->opts.exif_embedding);
    if (exif_data == NULL)
    {
#pragma omp critical
        std::cout << "No such embedding: " << this->opts.exif_embedding << std::endl;
        this->fallback_focal_length(view, viewport);
        return;
    }

    mve::image::ExifInfo exif = mve::image::exif_extract
        (exif_data->get_byte_pointer(), exif_data->get_byte_size(), false);
    FocalLengthEstimate estimate = sfm::extract_focal_length(exif);
    viewport->focal_length = estimate.first;

    /* Print warning in case extraction failed. */
#pragma omp critical
    if (estimate.second == FOCAL_LENGTH_FALLBACK_VALUE)
    {
        std::cout << "Warning: Using fallback focal length for view "
            << view->get_id() << "." << std::endl;
        if (!exif.camera_maker.empty() && !exif.camera_model.empty())
        {
            std::cout << "  Maker: " << exif.camera_maker
                << ", Model: " << exif.camera_model << std::endl;
        }
    }
}

void
Features::fallback_focal_length (mve::View::Ptr view, Viewport* viewport) const
{
#pragma omp critical
    std::cout << "Warning: No EXIF information for view "
        << view->get_id() << "!" << std::endl;

    mve::image::ExifInfo exif;
    FocalLengthEstimate estimate = sfm::extract_focal_length(exif);
    viewport->focal_length = estimate.first;
}

SFM_BUNDLER_NAMESPACE_END
SFM_NAMESPACE_END
