/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <QFormLayout>
#include <QFileDialog>
#include <QApplication>

#include "util/frame_timer.h"
#include "util/file_system.h"
#include "mve/image_io.h"
#include "mve/image_tools.h"

#include "scene_addins/addin_offscreen_renderer.h"
#include "scene_addins/camera_sequence.h"

AddinOffscreenRenderer::AddinOffscreenRenderer (void)
{
    this->output_frame_dir = new FileSelector();
    this->output_frame_dir->set_ellipsize(20);
    this->output_frame_dir->set_directory_mode();
    this->output_frame_dir->setToolTip("Set output frame directory");

    this->sequence_file = new FileSelector();
    this->sequence_file->set_ellipsize(20);
    this->sequence_file->setToolTip("Set input sequence file");

    this->width_spin = new QSpinBox();
    this->width_spin->setRange(1, 10000);
    this->width_spin->setValue(1280);

    this->height_spin = new QSpinBox();
    this->height_spin->setRange(1, 10000);
    this->height_spin->setValue(720);

    this->play_but = new QPushButton();
    this->play_but->setIcon(QIcon(":/images/icon_player_play.svg"));
    this->play_but->setIconSize(QSize(18, 18));
    this->play_but->setMaximumWidth(22);
    this->play_but->setToolTip("Play sequence");

    this->working_flag = false;

    /* Create UI elements. */
    QPushButton* offscreen_snapshot_but = new QPushButton
        (QIcon(":/images/icon_screenshot.svg"), "");
    QPushButton* offscreen_renderseq_but = new QPushButton(QIcon(":/images/icon_video.svg"), "");
    QPushButton* offscreen_display_but = new QPushButton(QIcon(":/images/icon_eye.svg"), "");
    offscreen_renderseq_but->setIconSize(QSize(18, 18));
    offscreen_display_but->setIconSize(QSize(18, 18));
    offscreen_renderseq_but->setToolTip("Offscreen render sequence to disc");
    offscreen_display_but->setToolTip("Display sequence splines in GUI");
    offscreen_snapshot_but->setToolTip("Save offscreen rendering to file");
    offscreen_snapshot_but->setIconSize(QSize(25, 25));
    offscreen_renderseq_but->setMaximumWidth(22);
    offscreen_display_but->setMaximumWidth(22);
    offscreen_snapshot_but->setMaximumWidth(32);

    QHBoxLayout* offscreen_hbox1 = new QHBoxLayout();
    offscreen_hbox1->addWidget(this->sequence_file);
    offscreen_hbox1->addWidget(offscreen_display_but);
    offscreen_hbox1->addWidget(this->play_but);
    QHBoxLayout* offscreen_hbox2 = new QHBoxLayout();
    offscreen_hbox2->addWidget(this->output_frame_dir);
    offscreen_hbox2->addWidget(offscreen_renderseq_but);
    QFormLayout* offscreen_rendering_layout = new QFormLayout();
    offscreen_rendering_layout->setVerticalSpacing(0);
    offscreen_rendering_layout->setHorizontalSpacing(5);
    offscreen_rendering_layout->addRow(tr("Width"), this->width_spin);
    offscreen_rendering_layout->addRow(tr("Height"), this->height_spin);
    QHBoxLayout* offscreen_rendering_hbox = new QHBoxLayout();
    offscreen_rendering_hbox->setSpacing(5);
    offscreen_rendering_hbox->addLayout(offscreen_rendering_layout);
    offscreen_rendering_hbox->addWidget(offscreen_snapshot_but);

    this->main_box = new QVBoxLayout();
    this->main_box->setSpacing(0);
    this->main_box->addLayout(offscreen_rendering_hbox);
    this->main_box->addLayout(offscreen_hbox1);
    this->main_box->addLayout(offscreen_hbox2);

    this->connect(offscreen_snapshot_but, SIGNAL(clicked()),
        this, SLOT(on_snapshot()));
    this->connect(offscreen_renderseq_but, SIGNAL(clicked()),
        this, SLOT(on_render_sequence()));
    this->connect(this->play_but, SIGNAL(clicked()),
        this, SLOT(on_play_sequence()));
    this->connect(offscreen_display_but, SIGNAL(clicked()),
        this, SLOT(on_display_sequence()));
}

/* ---------------------------------------------------------------- */

QWidget*
AddinOffscreenRenderer::get_sidebar_widget (void)
{
    return get_wrapper(this->main_box);
}

/* ---------------------------------------------------------------- */

void
AddinOffscreenRenderer::set_scene_camera (ogl::Camera* camera)
{
    this->camera = camera;
}

/* ---------------------------------------------------------------- */

void
AddinOffscreenRenderer::on_snapshot (void)
{
    QString qfname = QFileDialog::getSaveFileName();
    if (qfname.isEmpty())
        return;
    std::string fname = qfname.toStdString();
    mve::ByteImage::Ptr image = this->get_offscreen_image();
    try
    { mve::image::save_file(image, fname); }
    catch (std::exception& e)
    {
        this->show_error_box("Error saving image", e.what());
    }
}

/* ---------------------------------------------------------------- */

mve::ByteImage::Ptr
AddinOffscreenRenderer::get_offscreen_image (void)
{
    /* TODO: Make configurable. */
    float const znear = 0.1f;
    float const side = 0.05f;
    int const width = (int)this->width_spin->value();
    int const height = (int)this->height_spin->value();
    float const aspect = static_cast<float>(width) / static_cast<float>(height);

    /* Backup camera. */
    ogl::Camera camera_backup = *this->camera;

    /* Setup camera for offscreen rendering. */
    this->camera->width = width;
    this->camera->height = height;
    this->camera->z_near = znear;
    if (width > height)
    {

        this->camera->top = side;
        this->camera->right = side * aspect;
    }
    else
    {
        this->camera->right = side;
        this->camera->top = side / aspect;
    }
    this->camera->update_proj_mat();
    this->camera->update_inv_proj_mat();

    /* Request corresponding GL context. */
    this->request_context();

    /* Set new viewport. */
    glViewport(0, 0, width, height);

    GLuint bf[1], rb[2];
    glGenFramebuffers(1, bf);
    glGenRenderbuffers(2, rb);
    glBindFramebuffer(GL_FRAMEBUFFER, bf[0]);

    glBindRenderbuffer(GL_RENDERBUFFER, rb[0]);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, rb[1]);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_RENDERBUFFER, rb[0]);
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_RENDERBUFFER, rb[1]);
    this->repaint();

    /* Read color image from OpenGL. */
    mve::ByteImage::Ptr image = mve::ByteImage::create(width, height, 3);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, image->begin());
    mve::image::flip<uint8_t>(image, mve::image::FLIP_VERTICAL);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteRenderbuffers(2, rb);
    glDeleteFramebuffers(1, bf);

    /* Restore camera. */
    *this->camera = camera_backup;
    /* Restore viewport. */
    glViewport(0, 0, this->get_width(), this->get_height());

    return image;
}

/* ---------------------------------------------------------------- */

void
AddinOffscreenRenderer::on_render_sequence (void)
{
    this->on_play_sequence(true);
}

/* ---------------------------------------------------------------- */

void
AddinOffscreenRenderer::on_play_sequence (bool save)
{
    if (this->working_flag)
    {
        this->working_flag = false;
        return;
    }

    std::string frame_path = this->output_frame_dir->get_filename();
    std::string seq_file = this->sequence_file->get_filename();

    /* Load sequence. */
    CameraSequence sequence;
    try
    { sequence.read_file(seq_file); }
    catch (std::exception& e)
    {
        this->show_error_box("Error reading sequence",
            std::string("Cannot read sequence:\n") + e.what());
        return;
    }

    if (save && frame_path.empty())
    {
        this->show_error_box("Error saving frames",
            "No output path specified!");
        return;
    }

    /* Set frame timer for proper animation speed. */
    util::FrameTimer timer;
    timer.set_max_fps(sequence.get_fps());

    this->working_flag = true;
    this->play_but->setIcon(QIcon(":/images/icon_player_stop.svg"));
    while (this->working_flag && sequence.next_frame())
    {
        int frame = sequence.get_frame();

        /* Setup new camera parameters. */
        this->camera->pos = sequence.get_campos();
        this->camera->viewing_dir = (sequence.get_lookat() - sequence.get_campos()).normalized();
        this->camera->up_vec = sequence.get_upvec();
        this->camera->update_matrices();

        /* Render to disc if we have a path. */
        if (save)
        {
            std::string fname = frame_path + "/frame_"
                + util::string::get_filled(frame, 5, '0') + ".png";
            mve::ByteImage::Ptr image = this->get_offscreen_image();
            try
            { mve::image::save_file(image, fname); }
            catch (std::exception& e)
            {
                this->show_error_box("Error saving frame!", e.what());
                break;
            }
        }

        /* Render to GL widget. */
        this->repaint();
        timer.next_frame();

        QApplication::processEvents();
    }
    this->working_flag = false;
    this->play_but->setIcon(QIcon(":/images/icon_player_play.svg"));
}

/* ---------------------------------------------------------------- */

void
AddinOffscreenRenderer::on_display_sequence (void)
{
    /* Load sequence. */
    std::string seq_file = this->sequence_file->get_filename();
    CameraSequence sequence;
    try
    { sequence.read_file(seq_file); }
    catch (std::exception& e)
    {
        this->show_error_box("Error reading sequence",
            std::string("Cannot read sequence:\n") + e.what());
        return;
    }

    mve::TriangleMesh::Ptr mesh = mve::TriangleMesh::create();
    mve::TriangleMesh::VertexList& verts = mesh->get_vertices();
    mve::TriangleMesh::ColorList& colors = mesh->get_vertex_colors();
    math::Vec4f pos_color(0.0f, 1.0f, 0.0f, 1.0f);
    math::Vec4f lookat_color(1.0f, 1.0f, 1.0f, 1.0f);
    math::Vec4f pos_cp_color(1.0f, 0.0f, 0.0f, 1.0f);
    math::Vec4f lookat_cp_color(1.0f, 0.0f, 1.0f, 1.0f);

    for (std::size_t i = 0; i < sequence.get_splines().size(); ++i)
    {
        CameraSpline const& spline(sequence.get_splines()[i]);
        for (std::size_t j = 0; j < spline.cs.get_points().size(); ++j)
        {
            verts.push_back(spline.cs.get_points()[j]);
            colors.push_back(pos_cp_color);
        }
        for (std::size_t j = 0; j < spline.ls.get_points().size(); ++j)
        {
            verts.push_back(spline.ls.get_points()[j]);
            colors.push_back(lookat_cp_color);
        }
    }

    while (sequence.next_frame())
    {
        math::Vec3f pos = sequence.get_campos();
        math::Vec3f lookat = sequence.get_lookat();
        verts.push_back(pos);
        colors.push_back(pos_color);
        verts.push_back(lookat);
        colors.push_back(lookat_color);
    }

    emit this->mesh_generated(util::fs::basename(seq_file), mesh);
    this->repaint();
}
