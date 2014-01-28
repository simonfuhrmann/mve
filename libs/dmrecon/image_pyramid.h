#ifndef DMRECON_IMAGE_PYRAMID_H
#define DMRECON_IMAGE_PYRAMID_H

#include <vector>
#include <map>

#include "mve/scene.h"
#include "mve/view.h"
#include "mve/image_base.h"

#include "mve/camera.h"
#include "math/matrix.h"
#include "util/ref_ptr.h"
#include "dmrecon/defines.h"
#include "util/thread.h"
#include "util/thread_locks.h"

MVS_NAMESPACE_BEGIN

struct ImagePyramidLevel {
    mve::ImageBase::ConstPtr image;

    int width, height;
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
class ImagePyramid : public std::vector<ImagePyramidLevel> {
public:
    typedef util::RefPtr<ImagePyramid> Ptr;
    typedef util::RefPtr<ImagePyramid const> ConstPtr;
};

class ImagePyramidCache {
public:
    static ImagePyramid::ConstPtr get(mve::Scene::Ptr scene,
        mve::View::Ptr view, std::string embeddingName, int minLevel);
    static void cleanup();

private:
    static util::Mutex metadataMutex;
    static mve::Scene::Ptr cachedScene;
    static std::string cachedEmbedding;

    static std::map<int, ImagePyramid::Ptr> entries;
};

MVS_NAMESPACE_END

#endif
