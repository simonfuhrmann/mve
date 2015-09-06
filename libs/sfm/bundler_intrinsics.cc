/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include "sfm/bundler_intrinsics.h"
#include "sfm/extract_focal_length.h"

SFM_NAMESPACE_BEGIN
SFM_BUNDLER_NAMESPACE_BEGIN

void
Intrinsics::compute (mve::Scene::Ptr scene, ViewportList* viewports)
{
    if (scene == nullptr)
        throw std::invalid_argument("Null scene given");
    if (viewports == nullptr)
        throw std::invalid_argument("No viewports given");
    if (viewports->size() != scene->get_views().size())
        throw std::invalid_argument("Viewports/scene size mismatch");

    mve::Scene::ViewList const& views = scene->get_views();
    for (std::size_t i = 0; i < views.size(); ++i)
    {
        if (views[i] == nullptr)
            continue;

        switch (this->opts.intrinsics_source)
        {
            case FROM_EXIF:
                this->init_from_exif(views[i], &viewports->at(i));
                break;
            case FROM_VIEWS:
                this->init_from_views(views[i], &viewports->at(i));
                break;
            default:
                throw std::invalid_argument("Invalid intrinsics source");
        }
    }

    /* Print unknown camera models. */
    if (!this->unknown_cameras.empty())
    {
        std::cout << "Camera models not in database:" << std::endl;
        for (auto item : this->unknown_cameras)
            std::cout << "  " << item.first << ": " << item.second << std::endl;
    }
}

void
Intrinsics::init_from_exif (mve::View::Ptr view, Viewport* viewport)
{
    /* Try to get EXIF data. */
    if (this->opts.exif_embedding.empty())
    {
        std::cout << "Warning: No EXIF information for view "
            << view->get_id() << ", using fallback!" << std::endl;
        this->fallback_focal_length(viewport);
        return;
    }

    mve::ByteImage::Ptr exif_data = view->get_blob(this->opts.exif_embedding);
    if (exif_data == nullptr)
    {
        std::cout << "Warning: No EXIF embedding for view "
            << view->get_id() << ", using fallback!" << std::endl;
        this->fallback_focal_length(viewport);
        return;
    }

    mve::image::ExifInfo exif;
    FocalLengthEstimate estimate;
    try
    {
        exif = mve::image::exif_extract(exif_data->get_byte_pointer(),
            exif_data->get_byte_size(), false);
        estimate = sfm::extract_focal_length(exif);
        viewport->focal_length = estimate.first;
    }
    catch (std::exception& e)
    {
        std::cout << "Warning: " << e.what() << std::endl;
        estimate = sfm::extract_focal_length(mve::image::ExifInfo());
    }

    /* Print warning in case extraction failed. */
    if (estimate.second == FOCAL_LENGTH_FALLBACK_VALUE)
    {
        std::cout << "Warning: Using fallback focal length for view "
            << view->get_id() << "." << std::endl;
        if (!exif.camera_maker.empty() && !exif.camera_model.empty())
        {
            std::cout << "  Maker: " << exif.camera_maker
                << ", Model: " << exif.camera_model << std::endl;

            std::string key = exif.camera_maker + " " + exif.camera_model;
            this->unknown_cameras[key] += 1;
        }
    }
}

void
Intrinsics::init_from_views (mve::View::Ptr view, Viewport* viewport)
{
    mve::CameraInfo const& camera = view->get_camera();
    if (camera.flen == 0.0f)
    {
        std::cout << "Warning: View " << view->get_id()
            << " has zero focal length. Using fallback." << std::endl;
        this->fallback_focal_length(viewport);
        return;
    }

    /*
     * The current implementation sets the focal length from the view
     * but uses defaults for radial distortion and principle point.
     *
     * TODO: Also use pre-defined radial distortion and principal point.
     *
     * RD: To support radial distortion, read "camera.radial_distortion"
     *     from the view and set it here. It must be in PBA format.
     * PP: To support principle point, use CameraInfo::ppoint and set it in
     *     the viewport. Note that a new field is required for this, and it
     *     needs to be processed elsewhere.
     */
    viewport->focal_length = camera.flen;
    viewport->radial_distortion = 0.0f;
}

void
Intrinsics::fallback_focal_length (Viewport* viewport)
{
    mve::image::ExifInfo exif;
    FocalLengthEstimate estimate = sfm::extract_focal_length(exif);
    viewport->focal_length = estimate.first;
}

SFM_BUNDLER_NAMESPACE_END
SFM_NAMESPACE_END
