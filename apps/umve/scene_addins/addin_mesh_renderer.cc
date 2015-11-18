/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include "ogl/opengl.h"
#include "ogl/texture.h"
#include "ogl/mesh_renderer.h"

#include "math/vector.h"
#include "util/file_system.h"
#include "mve/mesh_io.h"
#include "mve/mesh_io_ply.h"
#include "mve/mesh_io_obj.h"
#include "mve/mesh_tools.h"
#include "mve/image_io.h"

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
    mve::TriangleMesh::Ptr mesh, std::string const& filename,
    ogl::Texture::Ptr texture)
{
    this->mesh_list->add(name, mesh, filename, texture);
}

void
AddinMeshesRenderer::load_mesh (std::string const& filename)
{
    std::vector<mve::TriangleMesh::Ptr> meshes;
    std::vector<ogl::Texture::Ptr> textures;

    std::string ext = util::string::lowercase(util::string::right(filename, 4));
    if (ext == ".obj")
    {
        std::vector<mve::geom::ObjModelPart> obj_model_parts;

        try
        {
            mve::geom::load_obj_mesh(filename, &obj_model_parts);
        }
        catch (std::exception& e)
        {
            this->show_error_box("Could not load mesh", e.what());
            return;
        }

        for (mve::geom::ObjModelPart const & obj_model_part : obj_model_parts)
        {
            ogl::Texture::Ptr texture = nullptr;
            if (obj_model_part.mesh->has_vertex_texcoords()
                && !obj_model_part.texture_filename.empty())
            {
                mve::ByteImage::Ptr image;
                try
                {
                    image = mve::image::load_file(
                        obj_model_part.texture_filename);
                }
                catch (std::exception& e)
                {
                    this->show_error_box("Could not load texture", e.what());
                    return;
                }

                texture = ogl::Texture::create();
                texture->upload(image);
                texture->bind();
            }

            textures.push_back(texture);
            meshes.push_back(obj_model_part.mesh);
        }
    }
    else
    {
        mve::TriangleMesh::Ptr mesh;
        try
        {
            mesh = mve::geom::load_mesh(filename);
        }
        catch (std::exception& e)
        {
            this->show_error_box("Could not load mesh", e.what());
            return;
        }
        meshes.push_back(mesh);
        textures.push_back(nullptr);
    }

    for (std::size_t i = 0; i < meshes.size(); ++i)
    {
        mve::TriangleMesh::Ptr mesh = meshes[i];
        if (!mesh->get_faces().empty())
            mesh->ensure_normals();

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

        std::string name = util::fs::basename(filename);
        if (textures[i] != nullptr)
        {
            name += " [part" + util::string::get_filled(i, 2) + "]";
            this->add_mesh(name, mesh, "", textures[i]);
        }
        else
        {
            this->add_mesh(name, mesh, filename);
        }
    }
}

void
AddinMeshesRenderer::paint_impl (void)
{
    this->state->surface_shader->bind();
    this->state->surface_shader->send_uniform("lighting",
        static_cast<int>(this->render_lighting_cb->isChecked()));
    this->state->texture_shader->bind();
    this->state->texture_shader->send_uniform("lighting",
        static_cast<int>(this->render_lighting_cb->isChecked()));

    /* Draw meshes. */
    QMeshList::MeshList& ml(this->mesh_list->get_meshes());
    for (std::size_t i = 0; i < ml.size(); ++i)
    {
        MeshRep& mr(ml[i]);
        if (!mr.active || mr.mesh == nullptr)
            continue;

        /* If the renderer is not yet created, do it now! */
        if (mr.renderer == nullptr)
        {
            mr.renderer = ogl::MeshRenderer::create(mr.mesh);
            if (mr.mesh->get_faces().empty())
                mr.renderer->set_primitive(GL_POINTS);
        }

        /* Determine shader to use:
         * - use texture shader if a texture is available
         * - use wireframe shader for points without normals
         * - use surface shader otherwise. */
        ogl::ShaderProgram::Ptr mesh_shader;
        if (mr.texture != nullptr)
            mesh_shader = this->state->texture_shader;
        else if (!mr.mesh->has_vertex_normals())
            mesh_shader = this->state->wireframe_shader;
        else
            mesh_shader = this->state->surface_shader;

        mesh_shader->bind();

        /* Setup shader to use mesh color or default color. */
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
        if (mr.renderer != nullptr)
        {
            mr.renderer->set_shader(mesh_shader);
            glPolygonOffset(1.0f, -1.0f);
            glEnable(GL_POLYGON_OFFSET_FILL);

            if (mr.texture != nullptr)
            {
                mr.texture->bind();
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
            }

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
