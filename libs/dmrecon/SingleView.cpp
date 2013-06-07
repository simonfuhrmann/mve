#include <cassert>

#include "util/filesystem.h"
#include "mve/imagefile.h"
#include "mve/imagetools.h"
#include "mve/plyfile.h"
#include "mve/view.h"
#include "dmrecon/defines.h"
#include "dmrecon/SingleView.h"

MVS_NAMESPACE_BEGIN

SingleView::SingleView(mve::View::Ptr _view)
    :
    view(_view)
{
    if ((view.get() == NULL) || (!view->is_camera_valid()))
        throw std::invalid_argument("NULL view");
    viewID = view->get_id();

    mve::MVEFileProxy const* proxy = 0;
    proxy = view->get_proxy("tonemapped");
    if (proxy == 0)
        proxy = view->get_proxy("undistorted");
    if (proxy == 0)
        throw std::invalid_argument("No color image found");

    mve::CameraInfo cam(view->get_camera());
    cam.fill_camera_pos(*this->camPos);
    cam.fill_world_to_cam(*this->worldToCam);

    this->width = proxy->width;
    this->height = proxy->height;

    this->img_pyramid.resize(1);

    // compute projection matrix
    cam.fill_calibration(*this->img_pyramid[0].proj, width, height);
    cam.fill_inverse_calibration(*this->img_pyramid[0].invproj, width, height);
}

void
SingleView::createImagePyramid()
{
    /* clear everything except original size */
    this->img_pyramid.resize(1);

    /* check if color image is present */
    if (!this->img_pyramid[0].image.get())
        throw util::Exception("No color image loaded.");

    /* create image pyramid */
    mve::ImageBase::Ptr img = this->img_pyramid[0].image;
    mve::ImageType type = img->get_type();
    int curr_width = img->width();
    int curr_height = img->height();
    mve::CameraInfo cam(view->get_camera());

    while (std::min(curr_width, curr_height) >= 30) {
        // adjust principal point
        if (curr_width % 2 == 1)
            cam.ppoint[0] = cam.ppoint[0] * float(curr_width)
                / float(curr_width + 1);
        if (curr_height % 2 == 1)
            cam.ppoint[1] = cam.ppoint[1] * float(curr_height)
                / float(curr_height + 1);
        if (type == mve::IMAGE_TYPE_UINT8)
            img = mve::image::rescale_half_size_gaussian<uint8_t>(img, 1.f);
        else if (type == mve::IMAGE_TYPE_FLOAT)
            img = mve::image::rescale_half_size_gaussian<float>(img, 1.f);
        else
            throw util::Exception("Invalid image type");

        this->img_pyramid.push_back(PyramidLevel());
        PyramidLevel & nextLevel = this->img_pyramid.back();
        nextLevel.image = img;

        // compute new projection matrix
        curr_width = img->width();
        curr_height = img->height();
        cam.fill_calibration(*nextLevel.proj, curr_width, curr_height);
        cam.fill_inverse_calibration(*nextLevel.invproj, curr_width, curr_height);
    }
}

void
SingleView::prepareRecon(float scale)
{
    // compute scale factor from scale
    this->scale_factor = 1.f / std::pow(2,scale);
    this->createFileName(scale);
    // scale image
    int scaled_width = this->scale_factor * this->width;
    int scaled_height = this->scale_factor * this->height;
    std::cout << "scaled image size: " << scaled_width
        << " x " << scaled_height << std::endl;

    mve::ImageBase::Ptr color_image = img_pyramid[0].image;
    mve::ImageType type = color_image->get_type();
    if (type == mve::IMAGE_TYPE_UINT8) {
        this->scaled_image = mve::ByteImage::create
            (scaled_width, scaled_height, 3);
        mve::image::rescale_gaussian<uint8_t>(color_image,
            this->scaled_image, 1.f);
    }
    else if (type == mve::IMAGE_TYPE_FLOAT) {
        this->scaled_image = mve::FloatImage::create
            (scaled_width, scaled_height, 3);
        mve::image::rescale_gaussian<float>(color_image,
            this->scaled_image, 1.f);
    }
    else
        throw util::Exception("Invalid image type");

    // compute projection matrix
    mve::CameraInfo cam(this->view->get_camera());
    cam.fill_calibration(*this->proj_scaled, scaled_width, scaled_height);
    cam.fill_inverse_calibration(*this->invproj_scaled, scaled_width,
        scaled_height);

    // create images for reconstruction
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
    if (level != 0 && level >= int(this->img_pyramid.size()))
        throw std::invalid_argument("Requested pyramid level does not exist");

    math::Vec3f ray = this->img_pyramid[level].invproj * math::Vec3f(x+0.5f, y+0.5f, 1.f);
    ray.normalize();
    math::Matrix3f rot(view->get_camera().rot);
    return rot.transposed() * ray;
}

math::Vec3f
SingleView::viewRayScaled(int x, int y) const
{
    assert(this->scaled_image.get());

    math::Vec3f ray = this->invproj_scaled * math::Vec3f(x+0.5f, y+0.5f, 1.f);
    ray.normalize();
    math::Matrix3f rot(view->get_camera().rot);
    return rot.transposed() * ray;
}

void
SingleView::loadColorImage(std::string const& name)
{
    /* load undistorted color image */
    mve::ImageBase::Ptr color_image = this->view->get_image(name);
    if (!color_image.get())
        throw util::Exception("No color image embedding found: ", name);
    assert(this->width == color_image->width());
    assert(this->height == color_image->height());

    int channels = color_image->channels();
    mve::ImageType type = color_image->get_type();

    switch (type)
    {
        case mve::IMAGE_TYPE_UINT8:
            if (channels == 2 || channels == 4)
                mve::image::reduce_alpha<uint8_t>(color_image);
            if (color_image->channels() == 1)
                color_image = mve::image::expand_grayscale<uint8_t>(color_image);
            break;

        case mve::IMAGE_TYPE_FLOAT:
            if (channels == 2 || channels == 4)
                mve::image::reduce_alpha<float>(color_image);
            if (color_image->channels() == 1)
                color_image = mve::image::expand_grayscale<float>(color_image);
            break;

        default:
            throw util::Exception("Invalid image type");
    }

    if (color_image->channels() != 3)
        throw std::invalid_argument("Image with invalid number of channels");

    img_pyramid.resize(1);
    img_pyramid[0].image = color_image;
}

bool
SingleView::pointInFrustum(math::Vec3f const & wp)
{
    math::Vec3f cp(this->worldToCam.mult(wp,1.f));
    // check whether point lies in front of camera
    if (cp[2] <= 0.f)
        return false;
    math::Vec3f sp(this->img_pyramid[0].proj * cp);
    float x = sp[0] / sp[2] - 0.5f;
    float y = sp[1] / sp[2] - 0.5f;
    if (x >= 0 && x <= width-1 && y >= 0 && y <= height-1)
        return true;
    return false;
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

    std::cout << "Saving ply file as " << plyname << std::endl;
    mve::geom::save_ply_view(plyname, view->get_camera(),
        this->depthImg, this->confImg, this->scaled_image);
    mve::geom::save_xf_file(xfname, view->get_camera());
}

MVS_NAMESPACE_END
