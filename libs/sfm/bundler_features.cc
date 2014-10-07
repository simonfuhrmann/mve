#include "util/timer.h"
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

    std::size_t num_views = viewports->size();
    std::size_t num_done = 0;
    std::size_t total_features = 0;

    /* Iterate the scene and compute features. */
#pragma omp parallel for schedule(dynamic,1)
    for (std::size_t i = 0; i < views.size(); ++i)
    {
#pragma omp critical
        {
            num_done += 1;
            float percent = (num_done * 1000 / num_views) / 10.0f;
            std::cout << "\rDetecting features, view " << num_done << " of "
                << num_views << " (" << percent << "%)..." << std::flush;
        }

        if (views[i] == NULL)
            continue;

        mve::View::Ptr view = views[i];
        mve::ByteImage::Ptr image
            = view->get_byte_image(this->opts.image_embedding);
        if (image == NULL)
            continue;


        /* Rescale image until maximum image size is met. */
        util::WallTimer timer;
        while (this->opts.max_image_size > 0
            && image->width() * image->height() > this->opts.max_image_size)
            image = mve::image::rescale_half_size<uint8_t>(image);

        /* Compute features for view. */
        Viewport* viewport = &viewports->at(i);
        viewport->features.set_options(this->opts.feature_options);
        viewport->features.compute_features(image);
        viewport->width = image->width();
        viewport->height = image->height();
        std::size_t num_feats = viewport->features.positions.size();

        /* Try to get an estimate for the focal length. */
        this->estimate_focal_length(view, viewport);

        /* Clean up unused embeddings. */
        image.reset();
        view->cache_cleanup();

#pragma omp critical
        {
            std::cout << "\rView ID "
                << util::string::get_filled(view->get_id(), 4, '0') << " ("
                << viewport->width << "x" << viewport->height << "), "
                << util::string::get_filled(num_feats, 5, ' ') << " features, "
                << "flen: " << viewport->focal_length << ", took "
                << timer.get_elapsed() << " ms." << std::endl;
            total_features += viewport->features.positions.size();
        }
    }

    std::cout << "\rComputed " << total_features << " features "
        << "for " << num_views << " views (average "
        << (total_features / num_views) << ")." << std::endl;
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
