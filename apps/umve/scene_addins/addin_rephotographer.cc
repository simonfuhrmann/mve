/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <iostream>
#include <QIcon>
#include <QPushButton>
#include <QApplication>

#include "mve/image.h"
#include "mve/image_tools.h"
#include "mve/view.h"
#include "mve/scene.h"
#include "mve/depthmap.h"

#include "guihelpers.h"
#include "scenemanager.h"
#include "scene_addins/addin_rephotographer.h"

AddinRephotographer::AddinRephotographer (void)
{
    this->rephoto_source = new QLineEdit();
    this->rephoto_color_dest = new QLineEdit();
    this->rephoto_depth_dest = new QLineEdit();

    QPushButton* rephoto_but = new QPushButton
        (QIcon(":/images/icon_screenshot.svg"), "Re-Photo current view");
    QPushButton* rephoto_all_but = new QPushButton
        (QIcon(":/images/icon_screenshot.svg"), "Re-Photo all views");
    rephoto_but->setIconSize(QSize(18, 18));
    rephoto_all_but->setIconSize(QSize(18, 18));

    this->rephoto_form = new QFormLayout();
    this->rephoto_form->setVerticalSpacing(0);
    this->rephoto_form->addRow("Source:", this->rephoto_source);
    this->rephoto_form->addRow(rephoto_but);
    this->rephoto_form->addRow(rephoto_all_but);
    this->rephoto_form->addRow("Color:", this->rephoto_color_dest);
    this->rephoto_form->addRow("Depth:", this->rephoto_depth_dest);

    this->rephoto_source->setText("undistorted");
    this->rephoto_color_dest->setText("rephoto-L0");
    this->rephoto_depth_dest->setText("rephoto-depth-L0");

    this->connect(rephoto_but, SIGNAL(clicked()),
        this, SLOT(on_rephoto()));
    this->connect(rephoto_all_but, SIGNAL(clicked()),
        this, SLOT(on_rephoto_all()));
}

QWidget*
AddinRephotographer::get_sidebar_widget (void)
{
    return get_wrapper(this->rephoto_form);
}

void
AddinRephotographer::set_scene_camera (ogl::Camera* camera)
{
    this->camera = camera;
}

void
AddinRephotographer::on_rephoto (void)
{
    mve::View::Ptr active_view = SceneManager::get().get_view();
    this->on_rephoto_view(active_view);
}

void
AddinRephotographer::on_rephoto_view (mve::View::Ptr view)
{
    if (view == nullptr)
    {
        this->show_error_box("Error", "No view selected!");
        return;
    }
    std::string source_name
        = this->rephoto_source->text().toStdString();
    std::string dest_color_name
        = this->rephoto_color_dest->text().toStdString();
    std::string dest_depth_name
        = this->rephoto_depth_dest->text().toStdString();

    if (dest_color_name.empty() && dest_depth_name.empty())
    {
        this->show_error_box("Error",
            "Neither output image nor depth specified");
        return;
    }
    if (!view->has_image(source_name))
    {
        this->show_error_box("Error", "Source embedding not available!");
        return;
    }

    std::cout << "Re-photographing view "
        << view->get_name() << "..." << std::endl;

    /* Backup camera. */
    ogl::Camera camera_backup = *this->camera;

    int const width = view->get_image_proxy(source_name)->width;
    int const height = view->get_image_proxy(source_name)->height;
    float const znear = 0.1f;
    float const zfar = 1000.0f;
    mve::CameraInfo const& camera_info = view->get_camera();

    camera_info.fill_gl_viewtrans(this->camera->view.begin());
    camera_info.fill_gl_projection(this->camera->proj.begin(),
        width, height, znear, zfar);

    /* Re-photograph. */
    this->request_context();
    glViewport(0, 0, width, height);
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    GLuint renderbuffer[2];
    glGenRenderbuffers(2, renderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer[0]);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer[1]);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_RENDERBUFFER, renderbuffer[0]);
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_RENDERBUFFER, renderbuffer[1]);
    this->repaint();

    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    /* Read color image from OpenGL. */
    mve::ByteImage::Ptr image = mve::ByteImage::create(width, height, 3);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, image->begin());
    mve::image::flip<uint8_t>(image, mve::image::FLIP_VERTICAL);

    /* Read depth buffer from OpenGL. */
    mve::FloatImage::Ptr depth = mve::FloatImage::create(width, height, 1);
    glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT,
        GL_FLOAT, depth->begin());
    mve::image::flip<float>(depth, mve::image::FLIP_VERTICAL);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteRenderbuffers(2, renderbuffer);
    glDeleteFramebuffers(1, &framebuffer);

    /* Restore camera and viewport */
    *this->camera = camera_backup;
    glViewport(0, 0, this->get_width(), this->get_height());
    this->repaint();
    QApplication::processEvents();

    /* Put re-photography into view as embedding. */
    if (!dest_color_name.empty())
        view->set_image(image, dest_color_name);

    /* Put depth buffer into view as embedding. */
    if (!dest_depth_name.empty())
    {
        /* Convert depth buffer to depth map. */
        for (float* ptr = depth->begin(); ptr != depth->end(); ++ptr)
            *ptr = (*ptr == 1.0f)
                ? 0.0f
                : (zfar * znear) / ((znear - zfar) * *ptr + zfar);

        /* Convert depthmap to MVE format. */
        math::Matrix3f inv_calib;
        camera_info.fill_inverse_calibration(*inv_calib, width, height);
        mve::image::depthmap_convert_conventions<float>(depth, inv_calib, true);
        view->set_image(depth, dest_depth_name);
    }

    view->save_view();
    SceneManager::get().refresh_view();
}

void
AddinRephotographer::on_rephoto_all (void)
{
    mve::Scene::Ptr scene = SceneManager::get().get_scene();

    std::string source_embedding_name
        = this->rephoto_source->text().toStdString();
    mve::Scene::ViewList& views = scene->get_views();
    std::size_t num_rephotographed = 0;
    for (size_t i = 0; i < views.size(); ++i)
    {
        mve::View::Ptr view = views[i];
        if (view == nullptr)
            continue;
        if (!view->has_image(source_embedding_name))
            continue;
        this->on_rephoto_view(view);
        num_rephotographed += 1;
        if (num_rephotographed % 10 == 0)
            scene->cache_cleanup();
    }
    scene->cache_cleanup();
    this->show_info_box("Info", "Re-Photographed "
        + util::string::get(num_rephotographed) + " views!");
}
