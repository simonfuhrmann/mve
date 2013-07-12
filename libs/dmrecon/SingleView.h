#ifndef DMRECON_SINGLEVIEW_H
#define DMRECON_SINGLEVIEW_H

#include <cassert>
#include <set>
#include <iostream>
#include <utility>

#include "math/vector.h"
#include "math/matrix.h"
#include "mve/view.h"
#include "mve/bundlefile.h"
#include "mve/image.h"
#include "dmrecon/defines.h"
#include "dmrecon/ImagePyramid.h"

MVS_NAMESPACE_BEGIN

class SingleView
{
public:
    typedef util::RefPtr<SingleView> Ptr;

    SingleView(mve::Scene::Ptr _scene, mve::View::Ptr _view);
    ~SingleView();

    void addFeature(std::size_t idx);
    std::vector<std::size_t> const& getFeatureIndices() const;
    int clampLevel(int level) const;
    mve::ImageBase::ConstPtr const& getScaledImg() const;
    mve::ImageBase::ConstPtr const& getPyramidImg(int level) const;
    mve::View::Ptr getMVEView() const;

    std::string createFileName(float scale) const;
    float footPrint(math::Vec3f const& point);
    float footPrintScaled(math::Vec3f const& point);
    math::Vec3f viewRay(int x, int y, int level) const;
    math::Vec3f viewRay(float x, float y, int level) const;
    math::Vec3f viewRayScaled(int x, int y) const;
    void loadColorImage(std::string const& name, int minLevel);
    bool pointInFrustum(math::Vec3f const& wp);
    void saveReconAsPly(std::string const& path, float scale) const;
    bool seesFeature(std::size_t idx) const;
    void prepareRecon(int _scale);
    math::Vec2f worldToScreen(math::Vec3f const& point, int level);
    math::Vec2f worldToScreenScaled(math::Vec3f const& point);

public:
    std::size_t viewID;
    math::Vec3f camPos;

    mve::FloatImage::Ptr depthImg;
    mve::FloatImage::Ptr normalImg;
    mve::FloatImage::Ptr dzImg;
    mve::FloatImage::Ptr confImg;

private:
    /** external camera parameters */
    math::Matrix4f worldToCam;

    /** feature indices */
    std::vector<std::size_t> featInd;

    /** mve scene */
    mve::Scene::Ptr scene;

    /** mve view */
    mve::View::Ptr view;

    bool has_target_level;

    /** the original image in different scales */
    ImagePyramid::ConstPtr img_pyramid;
    ImagePyramidLevel source_level, target_level;
    int minLevel;
};


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

inline mve::ImageBase::ConstPtr const&
SingleView::getPyramidImg(int level) const
{
    assert(this->img_pyramid->at(level).image != NULL);
    return this->img_pyramid->at(level).image;
}

inline mve::ImageBase::ConstPtr const&
SingleView::getScaledImg() const
{
    assert(this->has_target_level);
    return this->target_level.image;
}

inline std::string
SingleView::createFileName(float scale) const
{
    std::string fileName = "mvs-";
    fileName += util::string::get_filled(this->viewID, 4);
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

MVS_NAMESPACE_END

#endif
