/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include "guihelpers.h"

#include "scenemanager.h"
#include "scene_addins/addin_frusta_base.h"
#include "scene_addins/addin_frusta_scene_renderer.h"

AddinFrustaSceneRenderer::AddinFrustaSceneRenderer (void)
{
    this->render_frusta_cb = new QCheckBox("Draw camera frusta");
    this->render_viewdir_cb = new QCheckBox("Draw viewing direction");
    this->render_frusta_cb->setChecked(true);
    this->render_viewdir_cb->setChecked(true);

    this->frusta_size_slider = new QSlider();
    this->frusta_size_slider->setMinimum(1);
    this->frusta_size_slider->setMaximum(100);
    this->frusta_size_slider->setValue(10);
    this->frusta_size_slider->setOrientation(Qt::Horizontal);

    /* Create frusta rendering layout. */
    this->render_frusta_form = new QFormLayout();
    this->render_frusta_form->setVerticalSpacing(0);
    this->render_frusta_form->addRow(this->render_frusta_cb);
    this->render_frusta_form->addRow(this->render_viewdir_cb);
    this->render_frusta_form->addRow("Frusta Size:", this->frusta_size_slider);

    this->connect(&SceneManager::get(), SIGNAL(scene_bundle_changed()),
        this, SLOT(reset_frusta_renderer()));
    this->connect(&SceneManager::get(), SIGNAL(scene_selected(mve::Scene::Ptr)),
        this, SLOT(reset_frusta_renderer()));
    this->connect(&SceneManager::get(), SIGNAL(view_selected(mve::View::Ptr)),
        this, SLOT(reset_viewdir_renderer()));
    this->connect(this->frusta_size_slider, SIGNAL(valueChanged(int)),
        this, SLOT(reset_frusta_renderer()));
    this->connect(this->frusta_size_slider, SIGNAL(valueChanged(int)),
        this, SLOT(repaint()));
    this->connect(this->render_frusta_cb, SIGNAL(clicked()),
        this, SLOT(repaint()));
    this->connect(this->render_viewdir_cb, SIGNAL(clicked()),
        this, SLOT(repaint()));
}

QWidget*
AddinFrustaSceneRenderer::get_sidebar_widget (void)
{
    return get_wrapper(this->render_frusta_form);
}

void
AddinFrustaSceneRenderer::reset_frusta_renderer (void)
{
    this->frusta_renderer.reset();
}

void
AddinFrustaSceneRenderer::reset_viewdir_renderer (void)
{
    this->viewdir_renderer.reset();
}

void
AddinFrustaSceneRenderer::paint_impl (void)
{
    if (this->render_frusta_cb->isChecked())
    {
        if (this->frusta_renderer == nullptr)
            this->create_frusta_renderer();
        if (this->frusta_renderer != nullptr)
            this->frusta_renderer->draw();
    }

    if (this->render_viewdir_cb->isChecked())
    {
        if (this->viewdir_renderer == nullptr)
            this->create_viewdir_renderer();
        if (this->viewdir_renderer != nullptr)
            this->viewdir_renderer->draw();
    }
}

void
AddinFrustaSceneRenderer::create_frusta_renderer (void)
{
    if (this->state->scene == nullptr)
        return;

    float size = this->frusta_size_slider->value() / 100.0f;
    mve::TriangleMesh::Ptr mesh = mve::TriangleMesh::create();
    mve::Scene::ViewList const& views(this->state->scene->get_views());
    for (std::size_t i = 0; i < views.size(); ++i)
    {
        if (views[i].get() == nullptr)
            continue;

        mve::CameraInfo const& cam = views[i]->get_camera();
        if (cam.flen == 0.0f)
            continue;

        add_camera_to_mesh(cam, size, mesh);
    }

    this->frusta_renderer = ogl::MeshRenderer::create(mesh);
    this->frusta_renderer->set_shader(this->state->wireframe_shader);
    this->frusta_renderer->set_primitive(GL_LINES);
}

void
AddinFrustaSceneRenderer::create_viewdir_renderer (void)
{
    if (this->state->view == nullptr)
        return;

    math::Vec3f campos, viewdir;
    mve::CameraInfo const& cam(this->state->view->get_camera());
    cam.fill_camera_pos(*campos);
    cam.fill_viewing_direction(*viewdir);

    mve::TriangleMesh::Ptr mesh = mve::TriangleMesh::create();
    mve::TriangleMesh::VertexList& verts = mesh->get_vertices();
    mve::TriangleMesh::ColorList& colors = mesh->get_vertex_colors();
    verts.push_back(campos);
    verts.push_back(campos + viewdir * 100.0f);
    colors.push_back(math::Vec4f(1.0f, 1.0f, 0.0f, 1.0f));
    colors.push_back(math::Vec4f(1.0f, 1.0f, 0.0f, 1.0f));

    this->viewdir_renderer = ogl::MeshRenderer::create(mesh);
    this->viewdir_renderer->set_shader(this->state->wireframe_shader);
    this->viewdir_renderer->set_primitive(GL_LINES);
}
