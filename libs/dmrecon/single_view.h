/*
 * Copyright (C) 2015, Ronny Klowsky, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef DMRECON_SINGLE_VIEW_H
#define DMRECON_SINGLE_VIEW_H

#include <cassert>
#include <set>
#include <iostream>
#include <utility>
#include <memory>

#include "math/vector.h"
#include "math/matrix.h"
#include "mve/view.h"
#include "mve/image.h"
#include "dmrecon/defines.h"
#include "dmrecon/image_pyramid.h"

MVS_NAMESPACE_BEGIN

class SingleView
{
public:
    typedef std::shared_ptr<SingleView> Ptr;
    typedef std::shared_ptr<SingleView const> ConstPtr;

public:
    static Ptr create (mve::Scene::Ptr scene, mve::View::Ptr view,
        std::string const& embedding);
    ~SingleView();

    void addFeature(std::size_t idx);
    std::vector<std::size_t> const& getFeatureIndices() const;
    int clampLevel(int level) const;
    mve::ByteImage::ConstPtr const& getScaledImg() const;
    mve::ByteImage::ConstPtr const& getPyramidImg(int level) const;
    mve::View::Ptr getMVEView() const;

    std::string createFileName(float scale) const;
    float footPrint(math::Vec3f const& point);
    float footPrintScaled(math::Vec3f const& point);
    math::Vec3f viewRay(int x, int y, int level) const;
    math::Vec3f viewRay(float x, float y, int level) const;
    math::Vec3f viewRayScaled(int x, int y) const;
    void loadColorImage(int minLevel);
    bool pointInFrustum(math::Vec3f const& wp) const;
    void saveReconAsPly(std::string const& path, float scale) const;
    bool seesFeature(std::size_t idx) const;
    void prepareMasterView(int scale);
    math::Vec2f worldToScreen(math::Vec3f const& point, int level);
    math::Vec2f worldToScreenScaled(math::Vec3f const& point);
    std::size_t getViewID() const;

public:
    math::Vec3f camPos;
    mve::FloatImage::Ptr depthImg;
    mve::FloatImage::Ptr normalImg;
    mve::FloatImage::Ptr dzImg;
    mve::FloatImage::Ptr confImg;

private:
    /** Constructor is private, use the create() method for instantiation. */
    SingleView(mve::Scene::Ptr _scene, mve::View::Ptr _view,
        std::string const& _embedding);

private:
    math::Matrix4f worldToCam;
    mve::Scene::Ptr scene;
    mve::View::Ptr view;
    std::string embedding;

    /** feature indices */
    std::vector<std::size_t> featInd;

    bool has_target_level;

    /** The original image in different scales. */
    ImagePyramid::ConstPtr img_pyramid;
    ImagePyramidLevel source_level;
    ImagePyramidLevel target_level;
    int minLevel;
};

/* ------------------------ Implementation ------------------------ */

inline SingleView::Ptr
SingleView::create (mve::Scene::Ptr scene, mve::View::Ptr view,
    std::string const& embedding)
{
    return Ptr(new SingleView(scene, view, embedding));
}

inline void
SingleView::addFeature(std::size_t idx)
{
    featInd.push_back(idx);
}

inline std::vector<std::size_t> const &
SingleView::getFeatureIndices() const
{
    return featInd;
}

inline int
SingleView::clampLevel(int level) const
{
    if (level < minLevel)
        return minLevel;

    int maxLevel = this->img_pyramid->size() - 1;
    if (level > maxLevel)
        return maxLevel;

    return level;
}

inline mve::View::Ptr
SingleView::getMVEView() const
{
    return this->view;
}

inline mve::ByteImage::ConstPtr const&
SingleView::getPyramidImg(int level) const
{
    return this->img_pyramid->at(level).image;
}

inline mve::ByteImage::ConstPtr const&
SingleView::getScaledImg() const
{
    return this->target_level.image;
}

inline std::string
SingleView::createFileName(float scale) const
{
    std::string fileName = "mvs-";
    fileName += util::string::get_filled(this->getViewID(), 4);
    fileName += "-L";
    fileName += util::string::get(scale);
    return fileName;
}

inline float
SingleView::footPrint(math::Vec3f const& point)
{
    return (this->worldToCam.mult(point, 1)[2] * this->source_level.invproj[0]);
}

inline float
SingleView::footPrintScaled(math::Vec3f const& point)
{
    assert(this->has_target_level);
    return (this->worldToCam.mult(point, 1)[2] * this->target_level.invproj[0]);
}

inline bool
SingleView::seesFeature(std::size_t idx) const
{
    for (std::size_t i = 0; i < featInd.size(); ++i)
        if (featInd[i] == idx)
            return true;
    return false;
}

inline math::Vec2f
SingleView::worldToScreenScaled(math::Vec3f const& point)
{
    assert(this->has_target_level);

    math::Vec3f cp(this->worldToCam.mult(point,1.f));
    math::Vec3f sp = this->target_level.proj * cp;

    math::Vec2f res(sp[0] / sp[2] - 0.5f, sp[1] / sp[2] - 0.5f);
    return res;
}

inline math::Vec2f
SingleView::worldToScreen(math::Vec3f const& point, int level)
{
    math::Vec3f cp(this->worldToCam.mult(point,1.f));
    math::Vec3f sp = this->img_pyramid->at(level).proj * cp;

    math::Vec2f res(sp[0] / sp[2] - 0.5f, sp[1] / sp[2] - 0.5f);
    return res;
}

inline std::size_t
SingleView::getViewID() const
{
    return this->view->get_id();
}

MVS_NAMESPACE_END

#endif
