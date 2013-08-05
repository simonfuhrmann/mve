#include <cassert>
#include <limits>

#include "util/filesystem.h"
#include "mve/imagefile.h"
#include "mve/imagetools.h"
#include "mve/depthmap.h"
#include "mve/plyfile.h"
#include "mve/view.h"
#include "dmrecon/defines.h"
#include "dmrecon/single_view.h"

MVS_NAMESPACE_BEGIN

SingleView::SingleView(mve::Scene::Ptr _scene, mve::View::Ptr _view)
    : scene(_scene)
    , view(_view)
    , has_target_level(false)
    , minLevel(std::numeric_limits<int>::max())
{
    if ((view == NULL) || (!view->is_camera_valid()))
        throw std::invalid_argument("NULL view");
    viewID = view->get_id();

    mve::MVEFileProxy const* proxy = NULL;
    proxy = view->get_proxy("tonemapped");
    if (proxy == NULL)
        proxy = view->get_proxy("undistorted");
    if (proxy == NULL)
        throw std::invalid_argument("No color image found");

    mve::CameraInfo cam(view->get_camera());
    cam.fill_camera_pos(*this->camPos);
    cam.fill_world_to_cam(*this->worldToCam);

    this->source_level = ImagePyramidLevel(cam, proxy->width, proxy->height);
}

SingleView::~SingleView()
{
    source_level.image.reset();
    target_level.image.reset();
    img_pyramid.reset();
    ImagePyramidCache::cleanup();
}

void
SingleView::prepareRecon(int scale)
{
    this->target_level = (*this->img_pyramid)[scale];
    this->createFileName(scale);

    int scaled_width = this->target_level.width;
    int scaled_height = this->target_level.height;

    // create images for reconstruction
    this->depthImg = mve::FloatImage::create(scaled_width, scaled_height, 1);
    this->normalImg = mve::FloatImage::create(scaled_width, scaled_height, 3);
    this->dzImg = mve::FloatImage::create(scaled_width, scaled_height, 2);
    this->confImg = mve::FloatImage::create(scaled_width, scaled_height, 1);

    this->has_target_level = true;
}

math::Vec3f
SingleView::viewRay(int x, int y, int level) const
{
    return this->viewRay(float(x), float(y), level);
}

math::Vec3f
SingleView::viewRay(float x, float y, int level) const
{
    math::Vec3f ray = mve::geom::pixel_3dpos(x, y, 1.f, this->img_pyramid->at(level).invproj);
    math::Matrix3f rot(view->get_camera().rot);
    return rot.transposed() * ray;
}

math::Vec3f
SingleView::viewRayScaled(int x, int y) const
{
    assert(this->has_target_level);

    math::Vec3f ray = mve::geom::pixel_3dpos(x, y, 1.f, this->target_level.invproj);
    math::Matrix3f rot(view->get_camera().rot);
    return rot.transposed() * ray;
}

void
SingleView::loadColorImage(std::string const& name, int _minLevel)
{
    minLevel = _minLevel;
    img_pyramid = ImagePyramidCache::get(scene, view, name, minLevel);
}

bool
SingleView::pointInFrustum(math::Vec3f const & wp)
{
    math::Vec3f cp(this->worldToCam.mult(wp,1.f));
    // check whether point lies in front of camera
    if (cp[2] <= 0.f)
        return false;
    math::Vec3f sp(this->source_level.proj * cp);
    float x = sp[0] / sp[2] - 0.5f;
    float y = sp[1] / sp[2] - 0.5f;
    return x >= 0 && x <= this->source_level.width-1
            && y >= 0 && y <= this->source_level.height-1;
}

void
SingleView::saveReconAsPly(std::string const& path, float scale) const
{
    if (path.empty()) {
        throw std::invalid_argument("Empty path");
    }
    if (!util::fs::dir_exists(path.c_str()))
        util::fs::mkdir(path.c_str());

    std::string name(this->createFileName(scale));
    std::string plyname(path + "/" + name + ".ply");
    std::string xfname(path + "/" + name + ".xf");

    mve::geom::save_ply_view(plyname, view->get_camera(),
        this->depthImg, this->confImg, this->target_level.image);
    mve::geom::save_xf_file(xfname, view->get_camera());
}

MVS_NAMESPACE_END
