#include <iostream>
#include <string>

#include "mve/scene.h"
#include "sfm/bundler_features.h"

int
main (int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Syntax: " << argv[0] << " <scene>" << std::endl;
        return 1;
    }

    mve::Scene::Ptr scene = mve::Scene::create(argv[1]);

    sfm::bundler::Features::Options opts;
    opts.image_embedding = "original";
    opts.feature_embedding = "original-sift";
    opts.max_image_size = 2000000;
    opts.skip_saving_views = false;
    opts.force_recompute = true;

    sfm::bundler::Features bundler_features(opts);
    bundler_features.compute(scene, sfm::bundler::Features::SIFT_FEATURES);

    return 0;
}
