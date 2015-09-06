/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <iostream>

#include "util/file_system.h"
#include "util/exception.h"
#include "ogl/render_tools.h"

#include "scene_addins/addin_state.h"

AddinState::AddinState (void)
    : gl_widget(nullptr)
{
}

void
AddinState::repaint (void)
{
    if (this->gl_widget == nullptr)
        return;

    this->gl_widget->repaint();
}

void
AddinState::make_current_context (void)
{
    if (this->gl_widget == nullptr)
        return;

    this->gl_widget->makeCurrent();
}

namespace
{
    static void
    load_shaders_from_resources (ogl::ShaderProgram::Ptr prog, QString base)
    {
        {
            QFile file(base + ".frag");
            file.open(QIODevice::ReadOnly | QIODevice::Text);
            QByteArray code = file.readAll();
            std::string code_str(code.constData(), code.length());
            file.close();
            if (!code_str.empty())
                prog->load_frag_code(code_str);
            else
                prog->unload_frag();
        }

        {
            QFile file(base + ".geom");
            file.open(QIODevice::ReadOnly | QIODevice::Text);
            QByteArray code = file.readAll();
            std::string code_str(code.constData(), code.length());
            file.close();
            if (!code_str.empty())
                prog->load_geom_code(code_str);
            else
                prog->unload_geom();
        }

        {
            QFile file(base + ".vert");
            file.open(QIODevice::ReadOnly | QIODevice::Text);
            QByteArray code = file.readAll();
            std::string code_str(code.constData(), code.length());
            file.close();
            if (!code_str.empty())
                prog->load_vert_code(code_str);
            else
                prog->unload_vert();
        }
    }
}  /* namespace */

void
AddinState::load_shaders (void)
{
    /* Create shader programs only if necessary. */
    if (!this->surface_shader)
        this->surface_shader = ogl::ShaderProgram::create();
    if (!this->wireframe_shader)
        this->wireframe_shader = ogl::ShaderProgram::create();
    if (!this->texture_shader)
        this->texture_shader = ogl::ShaderProgram::create();
    if (!this->overlay_shader)
        this->overlay_shader = ogl::ShaderProgram::create();

    /* Setup search paths for shader files. */
    std::string home_dir = util::fs::get_home_dir();
    std::string binary_dir = util::fs::dirname(util::fs::get_binary_path());
    std::vector<std::string> shader_paths;
    shader_paths.push_back(binary_dir + "/shaders/");
    shader_paths.push_back(home_dir + "/.local/share/umve/shaders");
    shader_paths.push_back("/usr/local/share/umve/shaders/");
    shader_paths.push_back("/usr/share/umve/shaders/");

    bool found_surface = false;
    bool found_wireframe = false;
    bool found_texture = false;
    bool found_overlay = false;
    for (std::size_t i = 0; i < shader_paths.size(); ++i)
    {
        try
        {
            if (!found_surface)
                found_surface = this->surface_shader->try_load_all
                    (shader_paths[i] + "surface_330");
            if (!found_wireframe)
                found_wireframe = this->wireframe_shader->try_load_all
                    (shader_paths[i] + "wireframe_330");
            if (!found_texture)
                found_texture = this->texture_shader->try_load_all
                    (shader_paths[i] + "texture_330");
            if (!found_overlay)
                found_overlay = this->overlay_shader->try_load_all
                    (shader_paths[i] + "overlay_330");
        }
        catch (util::Exception& e)
        {
            std::cout << "Error while loading shaders from "
                << shader_paths[i] << "." << std::endl;
            throw e;
        }
    }

    /*
     * Shaders loaded later override those loaded earlier, so try
     * Qt Resources first and files afterwards.
     */
    if (!found_surface)
    {
        std::cout << "Using built-in surface shader." << std::endl;
        load_shaders_from_resources(this->surface_shader,
            ":/shaders/surface_330");
    }
    if (!found_wireframe)
    {
        std::cout << "Using built-in wireframe shader." << std::endl;
        load_shaders_from_resources(this->wireframe_shader,
            ":/shaders/wireframe_330");
    }
    if (!found_texture)
    {
        std::cout << "Using built-in texture shader." << std::endl;
        load_shaders_from_resources(this->texture_shader,
            ":/shaders/texture_330");
    }
    if (!found_overlay)
    {
        std::cout << "Using built-in overlay shader." << std::endl;
        load_shaders_from_resources(this->overlay_shader,
            ":/shaders/overlay_330");
    }
}

void
AddinState::send_uniform (ogl::Camera const& cam)
{
    /* Setup wireframe shader. */
    this->wireframe_shader->bind();
    this->wireframe_shader->send_uniform("viewmat", cam.view);
    this->wireframe_shader->send_uniform("projmat", cam.proj);

    /* Setup surface shader. */
    this->surface_shader->bind();
    this->surface_shader->send_uniform("viewmat", cam.view);
    this->surface_shader->send_uniform("projmat", cam.proj);

    /* Setup texture shader. */
    this->texture_shader->bind();
    this->texture_shader->send_uniform("viewmat", cam.view);
    this->texture_shader->send_uniform("projmat", cam.proj);

    /* Setup overlay shader. */
    this->overlay_shader->bind();
}

void
AddinState::init_ui (void)
{
    this->gui_renderer = ogl::create_fullscreen_quad(this->overlay_shader);
    this->gui_texture = ogl::Texture::create();
}

void
AddinState::clear_ui (int width, int height)
{
    this->ui_image = mve::ByteImage::create(width, height, 4);
    this->ui_image->fill(0);
}
