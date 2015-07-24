/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include "ogl/opengl.h"

#include "math/vector.h"
#include "util/file_system.h"
#include "mve/mesh_io.h"
#include "mve/mesh_io_ply.h"
#include "mve/mesh_tools.h"
#include "ogl/mesh_renderer.h"

#include "guihelpers.h"
#include "scene_addins/addin_mesh_renderer.h"

AddinMeshesRenderer::AddinMeshesRenderer (void)
{
    this->render_lighting_cb = new QCheckBox("Mesh lighting");
    this->render_wireframe_cb = new QCheckBox("Render wireframe");
    this->render_color_cb = new QCheckBox("Render mesh color");
    this->mesh_list = new QMeshList();

    this->render_lighting_cb->setChecked(true);
    this->render_color_cb->setChecked(true);

    this->render_meshes_box = new QVBoxLayout();
    this->render_meshes_box->setSpacing(0);
    this->render_meshes_box->addWidget(this->render_lighting_cb);
    this->render_meshes_box->addWidget(this->render_wireframe_cb);
    this->render_meshes_box->addWidget(this->render_color_cb);
    this->render_meshes_box->addSpacing(5);
    this->render_meshes_box->addWidget(this->mesh_list, 1);

    this->connect(this->render_lighting_cb, SIGNAL(clicked()),
        this, SLOT(repaint()));
    this->connect(this->render_wireframe_cb, SIGNAL(clicked()),
        this, SLOT(repaint()));
    this->connect(this->render_color_cb, SIGNAL(clicked()),
        this, SLOT(repaint()));
    this->connect(this->mesh_list, SIGNAL(signal_redraw()),
        this, SLOT(repaint()));
}

QWidget*
AddinMeshesRenderer::get_sidebar_widget (void)
{
    return get_wrapper(this->render_meshes_box);
}

void
AddinMeshesRenderer::add_mesh (std::string const& name,
    mve::TriangleMesh::Ptr mesh, std::string const& filename)
{
    this->mesh_list->add(name, mesh, filename);
}

void
AddinMeshesRenderer::load_mesh (std::string const& filename)
{
    mve::TriangleMesh::Ptr mesh;
    try
    {
        mesh = mve::geom::load_mesh(filename);
        if (!mesh->get_faces().empty())
            mesh->ensure_normals();
    }
    catch (std::exception& e)
    {
        this->show_error_box("Could not load mesh", e.what());
        return;
    }

    /* If there is a corresponding XF file, load it and transform mesh. */
    std::string xfname = util::fs::replace_extension(filename, "xf");
    if (util::fs::file_exists(xfname.c_str()))
    {
        try
        {
            math::Matrix4f ctw;
            mve::geom::load_xf_file(xfname, *ctw);
            mve::geom::mesh_transform(mesh, ctw);
        }
        catch (std::exception& e)
        {
            this->show_error_box("Error loading XF file", e.what());
        }
    }

    this->add_mesh(util::fs::basename(filename), mesh, filename);
}

void
AddinMeshesRenderer::paint_impl (void)
{
    this->state->surface_shader->bind();
    this->state->surface_shader->send_uniform("lighting",
        static_cast<int>(this->render_lighting_cb->isChecked()));

    /* Draw meshes. */
    QMeshList::MeshList& ml(this->mesh_list->get_meshes());
    for (std::size_t i = 0; i < ml.size(); ++i)
    {
        MeshRep& mr(ml[i]);
        if (!mr.active || mr.mesh == NULL)
            continue;

        /* If the renderer is not yet created, do it now! */
        if (mr.renderer == NULL)
        {
            mr.renderer = ogl::MeshRenderer::create(mr.mesh);
            if (mr.mesh->get_faces().empty())
                mr.renderer->set_primitive(GL_POINTS);
        }

        /* Determine shader to use. Use wireframe shader for points
         * without normals, use surface shader otherwise. */
        ogl::ShaderProgram::Ptr mesh_shader;
        if (mr.mesh->get_vertex_normals().empty())
            mesh_shader = this->state->wireframe_shader;
        else
            mesh_shader = this->state->surface_shader;

        //static_cast<int>(this->draw_mesh_lighting_cb.isChecked()));

        /* Setup shader to use mesh color or default color. */
        mesh_shader->bind();
        if (this->render_color_cb->isChecked() && mr.mesh->has_vertex_colors())
        {
            math::Vec4f null_color(0.0f);
            mesh_shader->send_uniform("ccolor", null_color);
        }
        else
        {
            // TODO: Make mesh color configurable.
            math::Vec4f default_color(0.7f, 0.7f, 0.7f, 1.0f);
            mesh_shader->send_uniform("ccolor", default_color);
        }

        /* If we have a valid renderer, draw it. */
        if (mr.renderer != NULL)
        {
            mr.renderer->set_shader(mesh_shader);
            glPolygonOffset(1.0f, -1.0f);
            glEnable(GL_POLYGON_OFFSET_FILL);
            mr.renderer->draw();
            glDisable(GL_POLYGON_OFFSET_FILL);

            if (this->render_wireframe_cb->isChecked())
            {
                this->state->wireframe_shader->bind();
                this->state->wireframe_shader->send_uniform("ccolor",
                    math::Vec4f(0.0f, 0.0f, 0.0f, 0.5f));
                mr.renderer->set_shader(this->state->wireframe_shader);
                glEnable(GL_BLEND);
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                mr.renderer->draw();
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                glDisable(GL_BLEND);
            }
        }
    }
}
