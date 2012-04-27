#ifndef SINGLEVIEW_H
#define SINGLEVIEW_H

#include <cassert>
#include <set>
#include <iostream>
#include <utility>

#include "math/vector.h"
#include "math/matrix.h"
#include "mve/view.h"
#include "mve/bundlefile.h"
#include "mve/image.h"
#include "defines.h"

MVS_NAMESPACE_BEGIN

class SingleView;
typedef util::RefPtr<SingleView> SingleViewPtr;
typedef std::vector<SingleViewPtr> SingleViewPtrList;
typedef std::set<SingleViewPtr> SingleViewPtrSet;

class SingleView
{
public:
    SingleView(mve::View::Ptr _view);

    void addFeature(std::size_t idx);
    std::vector<std::size_t> const& getFeatureIndices() const;
    int getMaxLevel() const;
    mve::ImageBase::Ptr getColorImg() const;
    mve::ImageBase::Ptr getScaledImg() const;
    mve::ImageBase::Ptr getPyramidImg(int level) const;

    std::string createFileName(float scale) const;
    void createImagePyramid();
    float footPrint(math::Vec3f const& point);
    math::Vec3f viewRay(std::size_t x, std::size_t y, int level = 0) const;
    math::Vec3f viewRay(float x, float y, int level) const;
    void loadColorImage(std::string const& name);
    bool pointInFrustum(math::Vec3f const& wp);
    void saveReconAsPly(std::string const& path, float scale) const;
    bool seesFeature(std::size_t idx) const;
    void prepareRecon(float _scale);
    math::Vec2f worldToScreen(math::Vec3f const& point, int level = 0);
    void writeReconImages(float scale);

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

    /** internal camera parameters */
    math::Matrix3f proj;
    math::Matrix3f invproj;

    /** feature indices */
    std::vector<std::size_t> featInd;

    /** mve view */
    mve::View::Ptr view;

    /** real image */
    mve::ImageBase::Ptr color_image;
    std::size_t width;
    std::size_t height;

    /** scaled image for reconstruction */
    float scale_factor;
    mve::ImageBase::Ptr scaled_image;
    std::size_t scaled_width;
    std::size_t scaled_height;
    math::Matrix3f proj_scaled;
    math::Matrix3f invproj_scaled;

    /** image pyramid */
    std::vector<mve::ImageBase::Ptr> img_pyramid;
    std::vector<std::size_t> widths;
    std::vector<std::size_t> heights;

    /** projective matrices for image pyramid */
    std::vector< math::Matrix3f > projs;
    std::vector< math::Matrix3f > invprojs;
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
SingleView::getMaxLevel() const
{
    return (this->img_pyramid.size() - 1);
}

inline mve::ImageBase::Ptr
SingleView::getColorImg() const
{
    return this->color_image;
}

inline mve::ImageBase::Ptr
SingleView::getPyramidImg(int level) const
{
    if (level >= int(this->img_pyramid.size())) {
        throw std::invalid_argument("Requested image does not exist.");
    }
    return this->img_pyramid[level];
}

inline mve::ImageBase::Ptr
SingleView::getScaledImg() const
{
    if (!this->scaled_image.get())
        throw std::runtime_error("No scaled image available.");
    return this->scaled_image;
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
    if (this->scaled_image.get())
        return (this->worldToCam.mult(point, 1)[2] * this->invproj_scaled[0]);
    else
        return (this->worldToCam.mult(point, 1)[2] * this->invproj[0]);
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
SingleView::worldToScreen(math::Vec3f const& point, int level)
{
    if (level != 0 && level >= int(this->img_pyramid.size()))
        throw std::invalid_argument("Requested pyramid level does not exist.");

    math::Vec3f cp(this->worldToCam.mult(point,1.f));
    math::Vec3f sp;
    if (level == 0) {
        if (this->scaled_image.get())
            sp = this->proj_scaled * cp;
        else
            sp = this->proj * cp;
    }
    else
        sp = this->projs[level] * cp;

    math::Vec2f res(sp[0] / sp[2] - 0.5f, sp[1] / sp[2] - 0.5f);
    return res;
}
 
MVS_NAMESPACE_END

#endif
