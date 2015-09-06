/*
 * Copyright (C) 2015, Ronny Klowsky, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <cassert>
#include <limits>

#include "util/file_system.h"
#include "mve/image_io.h"
#include "mve/image_tools.h"
#include "mve/depthmap.h"
#include "mve/mesh_io_ply.h"
#include "mve/view.h"
#include "dmrecon/defines.h"
#include "dmrecon/single_view.h"

MVS_NAMESPACE_BEGIN

SingleView::SingleView(mve::Scene::Ptr _scene,
    mve::View::Ptr _view, std::string const& _embedding)
    : scene(_scene)
    , view(_view)
    , embedding(_embedding)
    , has_target_level(false)
    , minLevel(std::numeric_limits<int>::max())
{
    /* Argument sanity checks. */
    if (scene == nullptr)
        throw std::invalid_argument("Null scene given");
    if (view == nullptr || !view->is_camera_valid())
        throw std::invalid_argument("Null view given");
    if (embedding.empty())
        throw std::invalid_argument("Empty embedding name");

    /* Initialize camera for the view. */
    mve::CameraInfo cam = view->get_camera();
    cam.fill_camera_pos(*this->camPos);
    cam.fill_world_to_cam(*this->worldToCam);

    /* Initialize view source level (original image size). */
    mve::View::ImageProxy const* proxy = view->get_image_proxy(_embedding);
    if (proxy == nullptr)
        throw std::invalid_argument("No color image found");
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
SingleView::loadColorImage(int _minLevel)
{
    minLevel = _minLevel;
    img_pyramid = ImagePyramidCache::get(this->scene, this->view, this->embedding, minLevel);
}

void
SingleView::prepareMasterView(int scale)
{
    /* Prepare target level (view is the master view). */
    this->target_level = (*this->img_pyramid)[scale];
    this->has_target_level = true;
    this->createFileName(scale);

    /* Create images for reconstruction. */
    int const scaled_width = this->target_level.width;
    int const scaled_height = this->target_level.height;
    this->depthImg = mve::FloatImage::create(scaled_width, scaled_height, 1);
    this->normalImg = mve::FloatImage::create(scaled_width, scaled_height, 3);
    this->dzImg = mve::FloatImage::create(scaled_width, scaled_height, 2);
    this->confImg = mve::FloatImage::create(scaled_width, scaled_height, 1);
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

bool
SingleView::pointInFrustum(math::Vec3f const& wp) const
{
    math::Vec3f cp = this->worldToCam.mult(wp, 1.0f);
    // check whether point lies in front of camera
    if (cp[2] <= 0.0f)
        return false;
    math::Vec3f sp = this->source_level.proj * cp;
    float x = sp[0] / sp[2] - 0.5f;
    float y = sp[1] / sp[2] - 0.5f;
    return x >= 0 && x <= this->source_level.width - 1
            && y >= 0 && y <= this->source_level.height - 1;
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
    std::string plyname = util::fs::join_path(path, name + ".ply");
    std::string xfname = util::fs::join_path(path, name + ".xf");

    mve::geom::save_ply_view(plyname, view->get_camera(),
        this->depthImg, this->confImg, this->target_level.image);
    mve::geom::save_xf_file(xfname, view->get_camera());
}

MVS_NAMESPACE_END
