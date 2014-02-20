#include "mve/image_tools.h"
#include "sfm/bundler_common.h"
#include "sfm/bundler_features.h"

SFM_NAMESPACE_BEGIN
SFM_BUNDLER_NAMESPACE_BEGIN

void
Features::compute (mve::Scene::Ptr scene, FeatureType type)
{
    /* Iterate the scene and check if recomputing is necessary. */
    mve::Scene::ViewList const& views = scene->get_views();
//#pragma omp parallel for schedule(dynamic)
    for (std::size_t i = 0; i < views.size(); ++i)
    {
        if (views[i] == NULL)
            continue;

        mve::View::Ptr view = views[i];
        if (!this->opts.force_recompute
            && view->has_data_embedding(this->opts.feature_embedding))
            continue;

        mve::ByteImage::Ptr image = view->get_byte_image
            (this->opts.image_embedding);
        if (image == NULL)
            continue;

        std::cout << "Computing features for view ID " << i
            << " (" << image->width() << "x" << image->height()
            << ")..." << std::endl;

        /* Rescale image until maximum image size is met. */
        bool was_scaled = false;
        while (image->width() * image->height() > this->opts.max_image_size)
        {
            image = mve::image::rescale_half_size<uint8_t>(image);
            was_scaled = true;
        }
        if (was_scaled)
        {
            std::cout << "  Scaled to " << image->width() << "x"
                << image->height() << " pixels." << std::endl;
        }

        /* Compute features for the image. */
        mve::ByteImage::Ptr descr_data;
        switch (type)
        {
            case SIFT_FEATURES:
            {
                Sift sift(this->opts.sift_options);
                sift.set_image(image);
                sift.process();
                Sift::Descriptors const& descriptors = sift.get_descriptors();

                std::cout << "  Computed " << descriptors.size()
                    << " descriptors." << std::endl;

                descr_data = descriptors_to_embedding(descriptors,
                    image->width(), image->height());
                break;
            }

            case SURF_FEATURES:
            {
                Surf surf(this->opts.surf_options);
                surf.set_image(image);
                surf.process();
                Surf::Descriptors const& descriptors = surf.get_descriptors();

                std::cout << "  Computed " << descriptors.size()
                    << " descriptors." << std::endl;

                descr_data = descriptors_to_embedding(descriptors,
                    image->width(), image->height());
                break;
            }

            default:
                throw std::runtime_error("Invalid switch case");
        }

        /* Save descriptors in view. */
        view->set_data(this->opts.feature_embedding, descr_data);
        if (!this->opts.skip_saving_views)
            view->save_mve_file();
        view->cache_cleanup();
    }
}

SFM_BUNDLER_NAMESPACE_END
SFM_NAMESPACE_END
