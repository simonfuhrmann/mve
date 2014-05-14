#include "mve/image_exif.h"
#include "mve/image_tools.h"
#include "sfm/bundler_common.h"
#include "sfm/extract_focal_length.h"
#include "sfm/bundler_features.h"

SFM_NAMESPACE_BEGIN
SFM_BUNDLER_NAMESPACE_BEGIN

void
Features::compute (mve::Scene::Ptr scene, FeatureType type,
    ViewportList* viewports)
{
    if (scene == NULL)
        throw std::invalid_argument("NULL scene given");

    if (viewports == NULL && this->opts.feature_embedding.empty())
        throw std::invalid_argument("No viewports or feature embedding given");

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

        /* Compute either SIFT or SURF features. */
        switch (type)
        {
            case SIFT_FEATURES:
                this->compute<Sift, 128>(view, viewport);
                break;

            case SURF_FEATURES:
                this->compute<Surf, 64>(view, viewport);
                break;

            default:
                throw std::invalid_argument("Invalid feature type");
        }

        /* Try to get an estimate for the focal length. */
        this->estimate_focal_length(view, viewport);
    }
}

template <>
Sift
Features::construct<Sift> (void) const
{
    return Sift(this->opts.sift_options);
}

template <>
Surf
Features::construct<Surf> (void) const
{
    return Surf(this->opts.surf_options);
}

template <typename FEATURE, int LEN>
void
Features::compute (mve::View::Ptr view, Viewport* viewport) const
{
    /* Check if descriptors can be loaded from embedding. */
    typename FEATURE::Descriptors descriptors;
    bool has_data = view->has_data_embedding(this->opts.feature_embedding);
    if (!this->opts.force_recompute && has_data)
    {
        /* If features exist but viewport is not given, skip computation. */
        if (viewport == NULL)
            return;

        /* Otherwise load descriptors from embedding. */
        mve::ByteImage::Ptr data = view->get_data(this->opts.feature_embedding);
        sfm::bundler::embedding_to_descriptors(data, &descriptors,
            &viewport->width, &viewport->height);
    }

    /*
     * Load color image either for computation of features or coloring
     * the loaded descriptors. In the latter case the image needs to be
     * rescaled to the image size descriptors have been computed from.
     */
    mve::ByteImage::Ptr img = view->get_byte_image(this->opts.image_embedding);
    if (descriptors.empty())
    {
        /* Rescale image until maximum image size is met. */
        while (img->width() * img->height() > this->opts.max_image_size)
            img = mve::image::rescale_half_size<uint8_t>(img);

        std::cout << "Computing features for view ID " << view->get_id()
            << " (" << img->width() << "x" << img->height()
            << ")..." << std::endl;

        /* Compute features. */
        FEATURE feature = this->construct<FEATURE>();
        feature.set_image(img);
        feature.process();
        descriptors = feature.get_descriptors();
        viewport->width = img->width();
        viewport->height = img->height();
    }
    else
    {
        /* Rescale image to exactly the descriptor image size. */
        while (img->width() > viewport->width
            && img->height() > viewport->height)
            img = mve::image::rescale_half_size<uint8_t>(img);
        if (img->width() != viewport->width
            || img->height() != viewport->height)
            throw std::runtime_error("Error rescaling image for descriptors");
    }

    /* Update feature embedding if requested. */
    if (!this->opts.feature_embedding.empty())
    {
        mve::ByteImage::Ptr descr_data = descriptors_to_embedding
            (descriptors, img->width(), img->height());
        view->set_data(this->opts.feature_embedding, descr_data);
        if (!this->opts.skip_saving_views)
            view->save_mve_file();
    }

    /* Initialize viewports. */
    if (viewport != NULL)
    {
        //viewport->image = img; // TMP
        viewport->descr_data.allocate(descriptors.size() * LEN);
        viewport->positions.resize(descriptors.size());
        viewport->colors.resize(descriptors.size());

        float* ptr = viewport->descr_data.begin();
        for (std::size_t i = 0; i < descriptors.size(); ++i, ptr += LEN)
        {
            typename FEATURE::Descriptor const& d = descriptors[i];
            std::copy(d.data.begin(), d.data.end(), ptr);
            viewport->positions[i] = math::Vec2f(d.x, d.y);
            img->linear_at(d.x, d.y, viewport->colors[i].begin());
        }
    }

    view->cache_cleanup();
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
