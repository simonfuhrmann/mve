/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <iostream>

#include <QTextEdit>
#include <QHBoxLayout>
#include <QDialog>
#include <QApplication>

#include "scenemanager.h"
#include "ogl/key_symbols.h"
#include "scene_addins/addin_selection.h"

size_t const MAX_POINTS_SHOWN = 100;

AddinSelection::AddinSelection (void)
    : selection_active(false)
    , camera(nullptr)
{
}

bool
AddinSelection::mouse_event (ogl::MouseEvent const& event)
{
    if (event.button == ogl::MOUSE_BUTTON_LEFT
        && event.type == ogl::MOUSE_EVENT_PRESS
        && (QApplication::keyboardModifiers() & Qt::ShiftModifier))
    {
        this->selection_active = true;
        this->rect_start_x = event.x;
        this->rect_start_y = event.y;
        this->rect_current_x = event.x;
        this->rect_current_y = event.y;
        this->state->ui_needs_redraw = true;
        return true;
    }

    if (!this->selection_active)
        return false;

    if (event.type == ogl::MOUSE_EVENT_MOVE)
    {
        this->rect_current_x = event.x;
        this->rect_current_y = event.y;
        this->state->ui_needs_redraw = true;
        return true;
    }

    if (event.button == ogl::MOUSE_BUTTON_LEFT
        && event.type == ogl::MOUSE_EVENT_RELEASE)
    {
        float left = (float)std::min(this->rect_current_x, this->rect_start_x);
        float right = (float)std::max(this->rect_current_x, this->rect_start_x);
        float top = (float)std::max(this->rect_current_y, this->rect_start_y);
        float bottom = (float)std::min(this->rect_current_y, this->rect_start_y);

        left = 2.0f * left / (float)this->get_width() - 1.0f;
        right = 2.0f * right / (float)this->get_width() - 1.0f;
        top = -2.0f * top / (float)this->get_height() + 1.0f;
        bottom = -2.0f * bottom / (float)this->get_height() + 1.0f;

        this->show_selection_info(left, right, top, bottom);

        this->selection_active = false;
        this->state->ui_needs_redraw = true;
    }

    return false;
}

void
AddinSelection::redraw_gui (void)
{
    if (!this->selection_active)
        return;

    int w = this->get_width();
    int h = this->get_height();

    int sx = std::min(this->rect_start_x, this->rect_current_x);
    int sy = std::min(this->rect_start_y, this->rect_current_y);
    int ex = std::max(this->rect_start_x, this->rect_current_x);
    int ey = std::max(this->rect_start_y, this->rect_current_y);
    sx = math::clamp(sx, 0, w-1);
    sy = math::clamp(sy, 0, h-1);
    ex = math::clamp(ex, 0, w-1);
    ey = math::clamp(ey, 0, h-1);

    for (int y = sy; y <= ey; ++y)
        for (int x = sx; x <= ex; ++x)
        {
            if (y == sy || y == ey || x == sx || x == ex)
            {
                this->state->ui_image->at(x, y, 0) = 255;
                this->state->ui_image->at(x, y, 1) = 0;
                this->state->ui_image->at(x, y, 2) = 0;
                this->state->ui_image->at(x, y, 3) = 255;
            }
            else
            {
                this->state->ui_image->at(x, y, 0) = 255;
                this->state->ui_image->at(x, y, 1) = 255;
                this->state->ui_image->at(x, y, 2) = 255;
                this->state->ui_image->at(x, y, 3) = 32;
            }
        }
}

void
AddinSelection::set_scene_camera (ogl::Camera *camera)
{
    this->camera = camera;
}

void
AddinSelection::show_selection_info (float left, float right, float top, float bottom)
{
    mve::Scene::Ptr scene = SceneManager::get().get_scene();

    if (scene == nullptr)
        return;

    /* Create a text representation of cameras and points in the region. */
    std::stringstream str_cameras, str_points;
    str_cameras << "<h2>Selected Cameras</h2>" << std::endl;

    /* Project camera positions to screen space and check if selected. */
    {
        bool found_camera = false;
        mve::Scene::ViewList const& views(scene->get_views());
        for (std::size_t i = 0; i < views.size(); ++i)
        {
            if (views[i] == nullptr || !views[i]->is_camera_valid())
                continue;

            math::Vec4f campos(1.0f);
            views[i]->get_camera().fill_camera_pos(*campos);
            campos = this->camera->view * campos;
            campos = this->camera->proj * campos;
            campos /= campos[3];

            if (campos[0] < left || campos[0] > right
                || campos[1] < top || campos[1] > bottom
                || campos[2] < -1.0 || campos[2] > 1.0)
                continue;

            found_camera = true;
            str_cameras << "View ID " << i << ", "
                << views[i]->get_name() << "<br/>" << std::endl;
        }

        if (!found_camera)
            str_cameras << "<p><i>No cameras selected!</i></p>" << std::endl;
    }

    /* Project bundle points to screen space and check if selected. */
    mve::Bundle::ConstPtr bundle;
    try { bundle = scene->get_bundle(); }
    catch (std::exception&) { }
    if (bundle != nullptr)
    {
        std::size_t num_points = 0;

        str_points << "<h2>Selected Bundle Points</h2>";

        mve::Bundle::Features const& features = bundle->get_features();
        mve::Scene::ViewList const& views(scene->get_views());
        for (std::size_t i = 0; i < features.size(); ++i)
        {
            math::Vec4f pos(1.0f);
            pos.copy(features[i].pos, 3);
            pos = this->camera->view * pos;
            pos = this->camera->proj * pos;
            pos /= pos[3];

            if (pos[0] < left || pos[0] > right
                || pos[1] < top || pos[1] > bottom
                || pos[2] < -1.0 || pos[2] > 1.0)
                continue;

            num_points += 1;

            if (num_points > MAX_POINTS_SHOWN)
                break;

            str_points << "Point ID " << i << ", visible in:<br/>" << std::endl;
            for (std::size_t j = 0; j < features[i].refs.size(); ++j)
            {
                mve::View::ConstPtr ref = views[features[i].refs[j].view_id];
                if (ref == nullptr)
                    continue;
                str_points << "&nbsp;&nbsp;View ID ";
                str_points << ref->get_id() << ", " << ref->get_name();
                if (!ref->is_camera_valid())
                    str_points << " (invalid)";
                str_points << "<br/>" << std::endl;
           }
           str_points << "<br/>" << std::endl;
        }

        if (num_points == 0)
            str_points.str("<h2>Selected Bundle Points</h2>"
                "<p><i>No points selected!</i></p>");
        else if (num_points > MAX_POINTS_SHOWN)
            str_points.str("<h2>Selected Bundle Points</h2>"
                "<p><i>More than 10 points selected!</i></p>");

    }

    QTextEdit* doc = new QTextEdit();
    doc->setHtml((str_cameras.str() + str_points.str()).c_str());
    doc->setReadOnly(true);
    QHBoxLayout* htmllayout = new QHBoxLayout();
    htmllayout->setMargin(10);
    htmllayout->addWidget(doc);

    QDialog* win = new QDialog();
    win->setWindowTitle("Selected Views / Cameras");
    win->setLayout(htmllayout);
    win->setWindowModality(Qt::WindowModal);
    win->show();
}
