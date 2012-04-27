#include <cassert>

#include "mve/imagefile.h"
#include "mve/imagetools.h"
#include "mve/plyfile.h"
#include "mve/view.h"
#include "util/fs.h"
#include "defines.h"
#include "SingleView.h"


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

    // compute projection matrix
    cam.fill_projection(*this->proj, width, height);
    cam.fill_inverse_projection(*this->invproj, width, height);
}

void
SingleView::createImagePyramid()
{
    /* clear everything */
    this->img_pyramid.clear();
    this->widths.clear();
    this->heights.clear();
    this->projs.clear();
    this->invprojs.clear();

    /* check if color image is present */
    if (!this->color_image.get())
        throw util::Exception("No color image loaded.");

    /* start with high-res color image */
    this->img_pyramid.push_back(this->color_image);
    this->widths.push_back(this->width);
    this->heights.push_back(this->height);
    this->projs.push_back(this->proj);
    this->invprojs.push_back(this->invproj);

    /* create image pyramid */
    mve::ImageType type = this->color_image->get_type();
    std::size_t curr_width = width;
    std::size_t curr_height = height;
    mve::ImageBase::Ptr img = this->color_image;
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
        this->img_pyramid.push_back(img);
        // compute new projection matrix
        curr_width = img->width();
        curr_height = img->height();
        math::Matrix3f mat;
        cam.fill_projection(*mat, curr_width, curr_height);
        this->projs.push_back(mat);
        cam.fill_inverse_projection(*mat, curr_width, curr_height);
        this->invprojs.push_back(mat);
    }
}

void
SingleView::prepareRecon(float scale)
{
    // compute scale factor from scale
    this->scale_factor = 1.f / std::pow(2,scale);
    this->createFileName(scale);
    // scale image
    this->scaled_width = this->scale_factor * this->width;
    this->scaled_height = this->scale_factor * this->height;
    std::cout << "scaled image size: " << this->scaled_width
        << " x " << this->scaled_height << std::endl;
    mve::ImageType type = this->color_image->get_type();
    if (type == mve::IMAGE_TYPE_UINT8) {
        this->scaled_image = mve::ByteImage::create
            (this->scaled_width, this->scaled_height, 3);
        mve::image::rescale_gaussian<uint8_t>(this->color_image,
            this->scaled_image, 1.f);
    }
    else if (type == mve::IMAGE_TYPE_FLOAT) {
        this->scaled_image = mve::FloatImage::create
            (this->scaled_width, this->scaled_height, 3);
        mve::image::rescale_gaussian<float>(this->color_image,
            this->scaled_image, 1.f);
    }
    else
        throw util::Exception("Invalid image type");

    // compute projection matrix
    mve::CameraInfo cam(this->view->get_camera());
    cam.fill_projection(*this->proj_scaled, this->scaled_width,
        this->scaled_height);
    cam.fill_inverse_projection(*this->invproj_scaled, this->scaled_width,
        this->scaled_height);

    // create images for reconstruction
    this->depthImg = mve::FloatImage::create(this->scaled_width,
        this->scaled_height, 1);
    this->normalImg = mve::FloatImage::create(this->scaled_width,
        this->scaled_height, 3);
    this->dzImg = mve::FloatImage::create(this->scaled_width,
        this->scaled_height, 2);
    this->confImg = mve::FloatImage::create(this->scaled_width,
        this->scaled_height, 1);
}

math::Vec3f
SingleView::viewRay(std::size_t x, std::size_t y, int level) const
{
    return this->viewRay(float(x), float(y), level);
}

math::Vec3f
SingleView::viewRay(float x, float y, int level) const
{
    if (level != 0 && level >= int(this->img_pyramid.size()))
        throw std::invalid_argument("Requested pyramid level does not exist");

    math::Vec3f ray;
    if (level == 0) {
        if (this->scaled_image.get())
            ray = this->invproj_scaled * math::Vec3f(x+0.5f, y+0.5f, 1.f);
        else
            ray = this->invproj * math::Vec3f(x+0.5f, y+0.5f, 1.f);
    }
    else {
        ray = this->invprojs[level] * math::Vec3f(x+0.5f, y+0.5f, 1.f);
    }
    ray.normalize();
    math::Matrix3f rot(view->get_camera().rot);
    return rot.transposed() * ray;
}

void
SingleView::loadColorImage(std::string const& name)
{
    /* load undistorted color image */
    this->color_image = this->view->get_image(name);
    if (!this->color_image.get())
        throw util::Exception("No color image embedding found: ", name);
    assert(this->width == this->color_image->width());
    assert(this->height == this->color_image->height());

    std::size_t channels = this->color_image->channels();
    mve::ImageType type = this->color_image->get_type();

    switch (type)
    {
        case mve::IMAGE_TYPE_UINT8:
            if (channels == 2 || channels == 4)
                mve::image::reduce_alpha<uint8_t>(this->color_image);
            if (this->color_image->channels() == 1)
                this->color_image = mve::image::expand_grayscale<uint8_t>(this->color_image);
            break;

        case mve::IMAGE_TYPE_FLOAT:
            if (channels == 2 || channels == 4)
                mve::image::reduce_alpha<float>(this->color_image);
            if (this->color_image->channels() == 1)
                this->color_image = mve::image::expand_grayscale<float>(this->color_image);
            break;

        default:
            throw util::Exception("Invalid image type");
    }

    if (this->color_image->channels() != 3)
        throw std::invalid_argument("Image with invalid number of channels");
}

bool
SingleView::pointInFrustum(math::Vec3f const & wp)
{
    math::Vec3f cp(this->worldToCam.mult(wp,1.f));
    // check whether point lies in front of camera
    if (cp[2] > 0.f)
        return false;
    math::Vec3f sp(this->proj * cp);
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

void
SingleView::writeReconImages(float scale)
{
    if (depthImg.get() == NULL)
        throw std::invalid_argument("No reconstruction available.");
    std::string name("depth-L");
    name += util::string::get(scale);
    view->set_image(name, this->depthImg);
    name = "dz-L";
    name += util::string::get(scale);
    view->set_image(name, this->dzImg);
    name = "conf-L";
    name += util::string::get(scale);
    view->set_image(name, this->confImg);
    if (this->scale_factor != 1.f) {
        name = "undist-L";
        name += util::string::get(scale);
        view->set_image(name, this->scaled_image);
    }
}

MVS_NAMESPACE_END
