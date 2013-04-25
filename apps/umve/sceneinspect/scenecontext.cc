#include <iostream>
#include <limits>

#include "util/filesystem.h"
#include "util/timer.h"
#include "util/frametimer.h"
#include "ogl/opengl.h"
#include "ogl/keysyms.h"
#include "ogl/events.h"
#include "ogl/rendertools.h"
#include "mve/depthmap.h"
#include "mve/trianglemesh.h"
#include "mve/meshtools.h"
#include "mve/plyfile.h"
#include "mve/scene.h"
#include "mve/imagetools.h"
#include "mve/imagefile.h"

#include "scenemanager.h"
#include "guihelpers.h"
#include "scenecontext.h"

SceneContext::SceneContext (void)
    : draw_worldaxis_cb("Draw world axis")
    , draw_sfmpoints_cb("Draw SfM points")
    , draw_camfrusta_cb("Draw camera frusta")
    , draw_curfrustum_cb("Draw viewing direction")
    , draw_mesh_lighting_cb("Mesh lighting")
    , clear_color(0, 0, 0)
    , clear_color_cb("Background color")
    , draw_wireframe_cb("Draw wireframe")
    , draw_meshcolor_cb("Draw mesh color")
    , dm_triangulate_but("DM triangulate")
{
    /* Init stuff. */
    this->rect_shift_pressed = false;
    this->rect_start_x = 0;
    this->rect_start_y = 0;
    this->rect_current_x = 0;
    this->rect_current_y = 0;
    this->ui_needs_update = true;

    /* Create selected view widget. */
    this->view = new SelectedView();
    this->meshlist = new QMeshList();

    this->offscreen_framedir.set_ellipsize(20);
    this->offscreen_seqfile.set_ellipsize(20);
    this->offscreen_framedir.set_directory_mode();
    this->offscreen_framedir.setToolTip("Set output frame directory");
    this->offscreen_seqfile.setToolTip("Set input sequence file");
    this->offscreen_width.setRange(1, 10000);
    this->offscreen_height.setRange(1, 10000);
    this->offscreen_width.setValue(1280);
    this->offscreen_height.setValue(720);
    this->offscreen_playbut.setIcon(QIcon(":/images/icon_player_play.svg"));
    this->offscreen_playbut.setIconSize(QSize(18, 18));
    this->offscreen_playbut.setMaximumWidth(22);
    this->offscreen_playbut.setToolTip("Play sequence");
    this->offscreen_working = false;
    this->offscreen_rephoto_source.setText("undistorted");
    this->offscreen_rephoto_color_dest.setText("rephoto");
    this->offscreen_rephoto_depth_dest.setText("rephoto-depth");

    this->dm_depth_disc.setMinimum(0.0f);
    this->dm_depth_disc.setMaximum(100.0f);
    this->dm_depth_disc.setValue(5.0f);

    this->draw_frusta_size.setMinimum(1);
    this->draw_frusta_size.setMaximum(100);
    this->draw_frusta_size.setValue(10);
    this->draw_frusta_size.setOrientation(Qt::Horizontal);

    /* Create scene rendering layout. */
    QFormLayout* rendering_layout = new QFormLayout();
    rendering_layout->setVerticalSpacing(0);
    rendering_layout->addRow(&this->draw_sfmpoints_cb);
    rendering_layout->addRow(&this->draw_worldaxis_cb);
    rendering_layout->addRow(&this->clear_color_cb);

    /* Create frusta rendering layout. */
    QFormLayout* frusta_rendering_layout = new QFormLayout();
    rendering_layout->setVerticalSpacing(0);
    frusta_rendering_layout->addRow(&this->draw_camfrusta_cb);
    frusta_rendering_layout->addRow(&this->draw_curfrustum_cb);
    frusta_rendering_layout->addRow("Frusta Size:", &this->draw_frusta_size);

    /* Create mesh rendering layout. */
    QVBoxLayout* mesh_rendering_layout = new QVBoxLayout();
    mesh_rendering_layout->setSpacing(0);
    mesh_rendering_layout->addWidget(&this->draw_mesh_lighting_cb);
    mesh_rendering_layout->addWidget(&this->draw_wireframe_cb);
    mesh_rendering_layout->addWidget(&this->draw_meshcolor_cb);

    /* Create offscreen rendering layout -- for VIDEO. */
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
    offscreen_hbox1->addWidget(&this->offscreen_seqfile);
    offscreen_hbox1->addWidget(offscreen_display_but);
    offscreen_hbox1->addWidget(&this->offscreen_playbut);
    QHBoxLayout* offscreen_hbox2 = new QHBoxLayout();
    offscreen_hbox2->addWidget(&this->offscreen_framedir);
    offscreen_hbox2->addWidget(offscreen_renderseq_but);
    QFormLayout* offscreen_rendering_layout = new QFormLayout();
    offscreen_rendering_layout->setVerticalSpacing(0);
    offscreen_rendering_layout->setHorizontalSpacing(5);
    offscreen_rendering_layout->addRow(tr("Width"), &this->offscreen_width);
    offscreen_rendering_layout->addRow(tr("Height"), &this->offscreen_height);
    QHBoxLayout* offscreen_rendering_hbox = new QHBoxLayout();
    offscreen_rendering_hbox->setSpacing(5);
    offscreen_rendering_hbox->addLayout(offscreen_rendering_layout);
    offscreen_rendering_hbox->addWidget(offscreen_snapshot_but);
    QVBoxLayout* offscreen_video_rendering_vbox = new QVBoxLayout();
    offscreen_video_rendering_vbox->setSpacing(0);
    offscreen_video_rendering_vbox->addLayout(offscreen_rendering_hbox);
    offscreen_video_rendering_vbox->addLayout(offscreen_hbox1);
    offscreen_video_rendering_vbox->addLayout(offscreen_hbox2);
    QCollapsible* offscreen_video_header = new QCollapsible
        ("Video Rendering", get_wrapper(offscreen_video_rendering_vbox));

    /* Create offscreen rendering layout -- for RE-PHOTO. */
    QPushButton* offscreen_rephoto_but = new QPushButton
        (QIcon(":/images/icon_screenshot.svg"), "Re-Photo current view");
    QPushButton* offscreen_rephoto_all_but = new QPushButton
        (QIcon(":/images/icon_screenshot.svg"), "Re-Photo all views");
    offscreen_rephoto_but->setIconSize(QSize(18, 18));
    offscreen_rephoto_all_but->setIconSize(QSize(18, 18));

    QFormLayout* offscreen_rephoto_layout = new QFormLayout();
    offscreen_rephoto_layout->setHorizontalSpacing(5);
    offscreen_rephoto_layout->setVerticalSpacing(1);
    offscreen_rephoto_layout->addRow("Source:",
        &this->offscreen_rephoto_source);
    offscreen_rephoto_layout->addRow(offscreen_rephoto_but);
    offscreen_rephoto_layout->addRow(offscreen_rephoto_all_but);
    offscreen_rephoto_layout->addRow("Color:",
        &this->offscreen_rephoto_color_dest);
    offscreen_rephoto_layout->addRow("Depth:",
        &this->offscreen_rephoto_depth_dest);
    QCollapsible* offscreen_rephoto_header = new QCollapsible
        ("Re-Photo Rendering", get_wrapper(offscreen_rephoto_layout));

    /* Create offscreen rendering layout -- Main Layout. */
    QVBoxLayout* offscreen_main_vbox = new QVBoxLayout();
    offscreen_main_vbox->addWidget(offscreen_video_header);
    offscreen_main_vbox->addWidget(offscreen_rephoto_header);
    offscreen_video_header->set_collapsed(true);
    offscreen_rephoto_header->set_collapsed(true);

    /* Create DM triangulate layout. */
    QFormLayout* dmtri_form = new QFormLayout();
    dmtri_form->setVerticalSpacing(0);
    dmtri_form->addRow(tr("Depthmap"), &this->dm_depthmap);
    dmtri_form->addRow(tr("Image"), &this->dm_colorimage);
    dmtri_form->addRow(tr("DD factor"), &this->dm_depth_disc);
    dmtri_form->addRow(&this->dm_triangulate_but);

    QCollapsible* rendering_header = new QCollapsible
        ("Scene Rendering", get_wrapper(rendering_layout));
    QCollapsible* frusta_rendering_header = new QCollapsible
        ("Frusta Rendering", get_wrapper(frusta_rendering_layout));
    QCollapsible* mesh_rendering_header = new QCollapsible
        ("Mesh Rendering", get_wrapper(mesh_rendering_layout));
    QCollapsible* offscreen_rendering_header = new QCollapsible
        ("Offscreen Rendering", get_wrapper(offscreen_main_vbox));
    QCollapsible* dmtri_header = new QCollapsible
        ("DM Triangulate", get_wrapper(dmtri_form));
    QCollapsible* meshes_header = new QCollapsible
        ("Meshes", this->meshlist);
    dmtri_header->set_collapsed(true);
    mesh_rendering_header->set_collapsed(true);
    frusta_rendering_header->set_collapsed(true);
    offscreen_rendering_header->set_collapsed(true);
    offscreen_rendering_header->set_content_indent(10);
    meshes_header->set_collapsible(false);

    /* Pack everything together. */
    QVBoxLayout* main_layout = new QVBoxLayout(this);
    main_layout->setSpacing(5);
    main_layout->addWidget(this->view, 0);
    main_layout->addWidget(rendering_header, 0);
    main_layout->addWidget(frusta_rendering_header, 0);
    main_layout->addWidget(mesh_rendering_header, 0);
    main_layout->addWidget(offscreen_rendering_header, 0);
    main_layout->addWidget(dmtri_header, 0);
    main_layout->addWidget(meshes_header, 1);

    /* Set default settings. */
    this->draw_worldaxis_cb.setChecked(true);
    this->draw_camfrusta_cb.setChecked(true);
    this->draw_curfrustum_cb.setChecked(true);
    this->draw_sfmpoints_cb.setChecked(true);
    this->draw_mesh_lighting_cb.setChecked(true);
    this->draw_wireframe_cb.setChecked(false);
    this->draw_meshcolor_cb.setChecked(true);

    this->apply_clear_color();

    /* Connect signals. */
    this->connect(&this->draw_worldaxis_cb, SIGNAL(toggled(bool)),
        this, SLOT(update_gl()));
    this->connect(&this->draw_sfmpoints_cb, SIGNAL(toggled(bool)),
        this, SLOT(update_gl()));
    this->connect(&this->draw_camfrusta_cb, SIGNAL(toggled(bool)),
        this, SLOT(update_gl()));
    this->connect(&this->draw_curfrustum_cb, SIGNAL(toggled(bool)),
        this, SLOT(update_gl()));
    this->connect(&this->draw_mesh_lighting_cb, SIGNAL(toggled(bool)),
        this, SLOT(update_gl()));
    this->connect(&this->draw_wireframe_cb, SIGNAL(toggled(bool)),
        this, SLOT(update_gl()));
    this->connect(&this->draw_meshcolor_cb, SIGNAL(toggled(bool)),
        this, SLOT(update_gl()));
    this->connect(this->meshlist, SIGNAL(signal_redraw()),
        this, SLOT(update_gl()));
    this->connect(&this->dm_triangulate_but, SIGNAL(clicked()),
        this, SLOT(on_dm_triangulate()));
    this->connect(&this->dm_depthmap, SIGNAL(activated(QString)),
        this, SLOT(select_colorimage(QString)));
    this->connect(&this->clear_color_cb, SIGNAL(clicked()),
        this, SLOT(on_set_clear_color()));
    this->connect(&this->draw_frusta_size, SIGNAL(valueChanged(int)),
        this, SLOT(on_frusta_size_changed()));
    this->connect(offscreen_snapshot_but, SIGNAL(clicked()),
        this, SLOT(on_offscreen_snapshot()));
    this->connect(offscreen_renderseq_but, SIGNAL(clicked()),
        this, SLOT(on_offscreen_render_sequence()));
    this->connect(&this->offscreen_playbut, SIGNAL(clicked()),
        this, SLOT(on_offscreen_play_sequence()));
    this->connect(offscreen_display_but, SIGNAL(clicked()),
        this, SLOT(on_offscreen_display_sequence()));
    this->connect(offscreen_rephoto_but, SIGNAL(clicked()),
        this, SLOT(on_offscreen_rephoto()));
    this->connect(offscreen_rephoto_all_but, SIGNAL(clicked()),
        this, SLOT(on_offscreen_rephoto_all()));
    this->connect(&SceneManager::get(), SIGNAL(scene_bundle_changed()),
        this, SLOT(on_recreate_sfm_renderer()));
}

/* ---------------------------------------------------------------- */

void
SceneContext::init_impl (void)
{
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK)
        std::cout << "Error initializing GLEW: " << glewGetErrorString(err)
            << std::endl;

    std::string shader_path = util::fs::get_path_component
        (util::fs::get_binary_path()) + "/shader/";

    this->surface_shader = ogl::ShaderProgram::create();
    this->surface_shader->load_all(shader_path + "surface_120");

    this->wireframe_shader = ogl::ShaderProgram::create();
    this->wireframe_shader->load_all(shader_path + "wireframe_120");

    this->texture_shader = ogl::ShaderProgram::create();
    this->texture_shader->load_all(shader_path + "texture_120");

    this->axis_renderer = ogl::create_axis_renderer(this->wireframe_shader);
    this->gui_renderer = ogl::create_fullscreen_quad(this->texture_shader);
    this->gui_texture = ogl::Texture::create();
}

/* ---------------------------------------------------------------- */

void
SceneContext::resize_impl (int old_width, int old_height)
{
    this->ogl::CameraTrackballContext::resize_impl(old_width, old_height);
    this->ui_needs_update = true;
}

/* ---------------------------------------------------------------- */

void
SceneContext::paint_impl (void)
{
    /* Set clear color and depth. */
    glClearColor(clear_color.red() / 255.0f,
        clear_color.green() / 255.0f,
        clear_color.blue() / 255.0f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // TODO: Make configurable through GUI
    //glPointSize(4.0f);
    //glLineWidth(2.0f);

    /* Setup wireframe shader. */
    this->wireframe_shader->bind();
    this->wireframe_shader->send_uniform("viewmat", this->camera.view);
    this->wireframe_shader->send_uniform("projmat", this->camera.proj);

    /* Setup surface shader. */
    this->surface_shader->bind();
    this->surface_shader->send_uniform("viewmat", this->camera.view);
    this->surface_shader->send_uniform("projmat", this->camera.proj);
    this->surface_shader->bind();
    this->surface_shader->send_uniform("lighting",
        static_cast<int>(this->draw_mesh_lighting_cb.isChecked()));

    /* Draw axis. */
    if (this->draw_worldaxis_cb.isChecked())
    {
        this->wireframe_shader->bind();
        this->wireframe_shader->send_uniform("ccolor", math::Vec4f(0.0f));
        this->axis_renderer->draw();
    }

    /* Draw meshes. */
    QMeshList::MeshList& ml(this->meshlist->get_meshes());
    for (std::size_t i = 0; i < ml.size(); ++i)
    {
        MeshRep& mr(ml[i]);
        if (!mr.active || !mr.mesh.get())
            continue;

        /* If the renderer is not yet created, do it now! */
        if (!mr.renderer.get())
        {
            mr.renderer = ogl::MeshRenderer::create(mr.mesh);
            if (mr.mesh->get_faces().empty())
                mr.renderer->set_primitive(GL_POINTS);
        }

        /* Determine shader to use. Use wireframe shader for points
         * without normals, use surface shader otherwise. */
        ogl::ShaderProgram::Ptr mesh_shader;
        if (mr.mesh->get_vertex_normals().empty())
            mesh_shader = this->wireframe_shader;
        else
            mesh_shader = this->surface_shader;

        /* Setup shader to use mesh color or default color. */
        mesh_shader->bind();
        if (this->draw_meshcolor_cb.isChecked() && mr.mesh->has_vertex_colors())
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
        if (mr.renderer.get())
        {
            mr.renderer->set_shader(mesh_shader);
            glPolygonOffset(1.0f, -1.0f);
            glEnable(GL_POLYGON_OFFSET_FILL);
            mr.renderer->draw();
            glDisable(GL_POLYGON_OFFSET_FILL);

            if (this->draw_wireframe_cb.isChecked())
            {
                this->wireframe_shader->bind();
                this->wireframe_shader->send_uniform("ccolor",
                    math::Vec4f(0.0f, 0.0f, 0.0f, 0.5f));
                mr.renderer->set_shader(this->wireframe_shader);
                glEnable(GL_BLEND);
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                mr.renderer->draw();
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                glDisable(GL_BLEND);
            }
        }
    }

    /* Draw SfM points. */
    if (this->draw_sfmpoints_cb.isChecked())
    {
        /* Try to create renderer. */
        if (!this->sfm_renderer.get())
            this->create_sfm_renderer();

        /* Try to render. */
        if (this->sfm_renderer.get())
        {
            this->wireframe_shader->bind();
            this->wireframe_shader->send_uniform("ccolor", math::Vec4f(0.0f));
            this->sfm_renderer->draw();
        }
    }

    glDisable(GL_BLEND);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);

    /* Draw camera frusta. */
    if (this->draw_camfrusta_cb.isChecked())
    {
        /* Try to create renderer. */
        if (!this->frusta_renderer.get())
        {
            float size = (float)this->draw_frusta_size.value() / 100.0f;
            this->create_frusta_renderer(size);
        }

        /* Try to render. */
        if (this->frusta_renderer.get())
        {
            this->wireframe_shader->bind();
            this->wireframe_shader->send_uniform("ccolor", math::Vec4f(0.0f));
            this->frusta_renderer->draw();
        }
    }

    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);

    /* Draw current frusta. */
    if (this->draw_curfrustum_cb.isChecked())
    {
        if (!this->current_frustum_renderer.get())
            this->create_current_frustum_renderer();

        if (this->current_frustum_renderer.get())
        {
            this->wireframe_shader->bind();
            this->wireframe_shader->send_uniform("ccolor", math::Vec4f(0.0f));
            this->current_frustum_renderer->draw();
        }
    }

    /* Draw UI. */
    if (this->gui_renderer.get())
    {
        this->update_ui();
        //glDepthFunc(GL_ALWAYS);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        this->gui_texture->bind();
        this->texture_shader->bind();
        this->gui_renderer->draw();
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        //glDepthFunc(GL_LESS);
    }
}

/* ---------------------------------------------------------------- */

void
SceneContext::reload_shaders (void)
{
    if (this->surface_shader.get())
        this->surface_shader->reload_all();
    if (this->wireframe_shader.get())
        this->wireframe_shader->reload_all();
    if (this->texture_shader.get())
        this->texture_shader->reload_all();
}

/* ---------------------------------------------------------------- */

void
SceneContext::load_file (std::string const& filename)
{
    mve::TriangleMesh::Ptr mesh;
    try
    {
        mesh = mve::geom::load_mesh(filename);
        //mve::geom::mesh_scale_and_center(model, true, false);
        if (!mesh->get_faces().empty())
            mesh->ensure_normals();
    }
    catch (std::exception& e)
    {
        this->print_error("Could not load mesh", e.what());
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
            this->print_error("Error loading XF file", e.what());
        }
    }

    this->meshlist->add(util::fs::get_file_component(filename),
        mesh, filename);
}

/* ---------------------------------------------------------------- */

void
SceneContext::create_frusta_renderer (float size)
{
    if (!this->scene.get())
        return;

    /* Color at camera center (start) and at the four corners (end). */
    math::Vec4f const frustum_start_color(0.5f, 0.5f, 0.5f, 1.0f);
    math::Vec4f const frustum_end_color(0.5f, 0.5f, 0.5f, 1.0f);

    mve::TriangleMesh::Ptr mesh(mve::TriangleMesh::create());
    mve::TriangleMesh::VertexList& verts(mesh->get_vertices());
    mve::TriangleMesh::ColorList& colors(mesh->get_vertex_colors());
    mve::TriangleMesh::FaceList& faces(mesh->get_faces());

    mve::Scene::ViewList const& views(this->scene->get_views());
    for (std::size_t i = 0; i < views.size(); ++i)
    {
        if (!views[i].get())
            continue;

        mve::CameraInfo const& cam = views[i]->get_camera();
        if (cam.flen == 0.0f)
            continue;

        /* Get camera position and transformations. */
        math::Vec3f campos;
        math::Matrix4f ctw;
        cam.fill_camera_pos(*campos);
        cam.fill_cam_to_world(*ctw);

        math::Vec3f cam_x(1.0f, 0.0f, 0.0f);
        math::Vec3f cam_y(0.0f, 1.0f, 0.0f);
        math::Vec3f cam_z(0.0f, 0.0f, 1.0f);
        cam_x = ctw.mult(cam_x, 0.0f);
        cam_y = ctw.mult(cam_y, 0.0f);
        cam_z = ctw.mult(cam_z, 0.0f);

        std::size_t idx(verts.size());

        /* Vertices, colors and faces for the frustum. */
        verts.push_back(campos);
        colors.push_back(frustum_start_color);
        for (int j = 0; j < 4; ++j)
        {
            math::Vec3f corner = campos + size * cam_z
                + cam_x * size / (2.0f * cam.flen) * (j & 1 ? -1.0f : 1.0f)
                + cam_y * size / (2.0f * cam.flen) * (j & 2 ? -1.0f : 1.0f);
            verts.push_back(corner);
            colors.push_back(frustum_end_color);
            faces.push_back(idx + 0); faces.push_back(idx + 1 + j);
        }
        faces.push_back(idx + 1); faces.push_back(idx + 2);
        faces.push_back(idx + 2); faces.push_back(idx + 4);
        faces.push_back(idx + 4); faces.push_back(idx + 3);
        faces.push_back(idx + 3); faces.push_back(idx + 1);

        /* Vertices, colors and faces for the local coordinate system. */
        verts.push_back(campos);
        verts.push_back(campos + (size * 0.5f) * cam_x);
        verts.push_back(campos);
        verts.push_back(campos + (size * 0.5f) * cam_y);
        verts.push_back(campos);
        verts.push_back(campos + (size * 0.5f) * cam_z);
        colors.push_back(math::Vec4f(1.0f, 0.0f, 0.0f, 1.0f));
        colors.push_back(math::Vec4f(1.0f, 0.0f, 0.0f, 1.0f));
        colors.push_back(math::Vec4f(0.0f, 1.0f, 0.0f, 1.0f));
        colors.push_back(math::Vec4f(0.0f, 1.0f, 0.0f, 1.0f));
        colors.push_back(math::Vec4f(0.0f, 0.0f, 1.0f, 1.0f));
        colors.push_back(math::Vec4f(0.0f, 0.0f, 1.0f, 1.0f));
        faces.push_back(idx + 5); faces.push_back(idx + 6);
        faces.push_back(idx + 7); faces.push_back(idx + 8);
        faces.push_back(idx + 9); faces.push_back(idx + 10);
    }

    this->frusta_renderer = ogl::MeshRenderer::create(mesh);
    this->frusta_renderer->set_shader(this->wireframe_shader);
    this->frusta_renderer->set_primitive(GL_LINES);
}

/* ---------------------------------------------------------------- */

void
SceneContext::create_current_frustum_renderer (void)
{
    if (!this->view->get_view().get())
        return;

    mve::TriangleMesh::Ptr mesh(mve::TriangleMesh::create());
    mve::TriangleMesh::VertexList& verts(mesh->get_vertices());
    mve::TriangleMesh::ColorList& colors(mesh->get_vertex_colors());

    mve::CameraInfo const& cam(this->view->get_view()->get_camera());

    math::Vec3f campos;
    math::Vec3f viewdir;
    cam.fill_camera_pos(*campos);
    cam.fill_viewing_direction(*viewdir);

    verts.push_back(campos);
    verts.push_back(campos + viewdir * 100.0f);
    colors.push_back(math::Vec4f(1.0f, 1.0f, 0.0f, 1.0f));
    colors.push_back(math::Vec4f(1.0f, 1.0f, 0.0f, 1.0f));

    this->current_frustum_renderer = ogl::MeshRenderer::create(mesh);
    this->current_frustum_renderer->set_shader(this->wireframe_shader);
    this->current_frustum_renderer->set_primitive(GL_LINES);
}

/* ---------------------------------------------------------------- */

void
SceneContext::create_sfm_renderer (void)
{
    if  (!this->scene.get())
        return;

    try
    {
        mve::BundleFile::ConstPtr bundle(this->scene->get_bundle());
        mve::TriangleMesh::Ptr mesh(bundle->get_points_mesh());
        this->sfm_renderer = ogl::MeshRenderer::create(mesh);
        this->sfm_renderer->set_shader(this->wireframe_shader);
        this->sfm_renderer->set_primitive(GL_POINTS);
    }
    catch (std::exception& e)
    {
        // This should not trigger a redraw
        this->draw_sfmpoints_cb.blockSignals(true);
        this->draw_sfmpoints_cb.setChecked(false);
        this->draw_sfmpoints_cb.blockSignals(false);
        this->print_error("Error loading bundle", e.what());
    }
}

/* ---------------------------------------------------------------- */

void
SceneContext::set_view (mve::View::Ptr view)
{
    this->view->set_view(view);
    this->current_frustum_renderer.reset();
    this->view->fill_embeddings(this->dm_depthmap, mve::IMAGE_TYPE_FLOAT);
    this->view->fill_embeddings(this->dm_colorimage, mve::IMAGE_TYPE_UINT8);
    this->update_gl();
}

/* ---------------------------------------------------------------- */

void
SceneContext::set_scene (mve::Scene::Ptr scene)
{
    this->scene = scene;
    this->sfm_renderer.reset();
    this->frusta_renderer.reset();
    this->current_frustum_renderer.reset();
    this->update_gl();
}

/* ---------------------------------------------------------------- */

void
SceneContext::on_dm_triangulate (void)
{
    if (this->view == 0)
        return;

    float dd_factor = this->dm_depth_disc.value();
    std::string embedding = this->dm_depthmap.currentText().toStdString();
    std::string colorimage = this->dm_colorimage.currentText().toStdString();

    mve::View::Ptr view(this->view->get_view());
    if (!view.get())
    {
        this->print_error("Error triangulating", "No view available");
        return;
    }

    /* Fetch depthmap and color image. */
    mve::FloatImage::Ptr dm(view->get_float_image(embedding));
    if (!dm.get())
    {
        this->print_error("Error triangulating",
            "Depthmap not available: " + embedding);
        return;
    }
    mve::ByteImage::Ptr ci(view->get_byte_image(colorimage));
    mve::CameraInfo const& cam(view->get_camera());

    /* Triangulate mesh. */
    util::ClockTimer timer;
    mve::TriangleMesh::Ptr mesh;
    try
    { mesh = mve::geom::depthmap_triangulate(dm, ci, cam, dd_factor); }
    catch (std::exception& e)
    {
        this->print_error("Error triangulating", e.what());
        return;
    }
    std::cout << "Triangulating took " << timer.get_elapsed() << "ms" << std::endl;

    /* Save temp mesh. */
    //mve::geom::save_mesh(mesh, "/tmp/depthmap.ply");

    this->meshlist->add(view->get_name() + "-" + embedding, mesh);
    this->update_gl();
}

/* ---------------------------------------------------------------- */

void
SceneContext::print_error (std::string const& error, std::string const& message)
{
    QMessageBox::critical(this, error.c_str(), message.c_str());
}

void
SceneContext::print_info (std::string const& title, std::string const& message)
{
    QMessageBox::information(this, title.c_str(), message.c_str());
}

void
SceneContext::print_html (std::string const& title, std::string const& message)
{
#if 0
    QMessageBox* mbox = new QMessageBox();
    mbox->setTextFormat(Qt::RichText);
    mbox->setWindowTitle(title.c_str());
    mbox->setText(message.c_str());
    mbox->show();
#else
    QTextEdit* doc = new QTextEdit();
    doc->setHtml(message.c_str());
    doc->setReadOnly(true);
    QHBoxLayout* htmllayout = new QHBoxLayout();
    htmllayout->setMargin(10);
    htmllayout->addWidget(doc);

    QDialog* win = new QDialog();
    win->setWindowTitle(title.c_str());
    win->setLayout(htmllayout);
    win->setWindowModality(Qt::WindowModal);
    win->show();
#endif
}

/* ---------------------------------------------------------------- */

void
SceneContext::select_colorimage (QString name)
{
    if (!this->view || !this->view->get_view().get())
        return;

    std::string const depth_str("depth-");
    std::string const undist_str("undist-");
    std::string depthmap = name.toStdString();

    std::size_t pos = depthmap.find(depth_str);
    if (pos == std::string::npos)
        return;

    if (depthmap == "depth-L0")
        depthmap = "undistorted";
    else
    {
        depthmap.replace(depthmap.begin() + pos,
            depthmap.begin() + pos + depth_str.size(),
            undist_str.begin(), undist_str.end());
    }

    for (int i = 0; i < this->dm_colorimage.count(); ++i)
    {
        if (this->dm_colorimage.itemText(i).toStdString() == depthmap)
        {
            this->dm_colorimage.setCurrentIndex(i);
            break;
        }
    }
}

/* ---------------------------------------------------------------- */

void
SceneContext::apply_clear_color (void)
{
    QPalette pal;
    pal.setColor(QPalette::Base, this->clear_color);
    this->clear_color_cb.setPalette(pal);
}

/* ---------------------------------------------------------------- */

void
SceneContext::on_set_clear_color (void)
{
    this->clear_color_cb.setChecked(false);
    QColor newcol = QColorDialog::getColor(this->clear_color, this);
    if (!newcol.isValid())
        return;

    this->clear_color = newcol;
    this->apply_clear_color();
    this->update_gl();
}

/* ---------------------------------------------------------------- */

void
SceneContext::on_frusta_size_changed (void)
{
    this->frusta_renderer.reset();
    this->update_gl();
}

/* ---------------------------------------------------------------- */

void
SceneContext::on_recreate_sfm_renderer (void)
{
    if (!this->draw_sfmpoints_cb.isChecked())
        return;
    this->create_sfm_renderer();
}

/* ---------------------------------------------------------------- */

void
SceneContext::update_ui (void)
{
    if (!this->ui_needs_update)
        return;

#if 1

    //std::cout << "Updating UI..." << std::endl;

    int w = this->get_width();
    int h = this->get_height();
    this->ui_image = mve::ByteImage::create(w, h, 4);
    this->ui_image->fill(0);

    /* Draw rectangle. */
    if (this->rect_shift_pressed
        && this->rect_current_x != this->rect_start_x
        && this->rect_current_y != this->rect_start_y)
    {
        int sx = std::min(this->rect_start_x, this->rect_current_x);
        int sy = std::min(this->rect_start_y, this->rect_current_y);
        int ex = std::max(this->rect_start_x, this->rect_current_x);
        int ey = std::max(this->rect_start_y, this->rect_current_y);
        sx = math::algo::clamp(sx, 0, w-1);
        sy = math::algo::clamp(sy, 0, h-1);
        ex = math::algo::clamp(ex, 0, w-1);
        ey = math::algo::clamp(ey, 0, h-1);

        for (int y = sy; y <= ey; ++y)
            for (int x = sx; x <= ex; ++x)
            {
                if (y == sy || y == ey || x == sx || x == ex)
                {
                    this->ui_image->at(x, y, 0) = 255;
                    this->ui_image->at(x, y, 3) = 255;
                }
                else
                {
                    this->ui_image->at(x, y, 0) = 255;
                    this->ui_image->at(x, y, 1) = 255;
                    this->ui_image->at(x, y, 2) = 255;
                    this->ui_image->at(x, y, 3) = 32;
                }
            }
        //mve::image::save_file(this->ui_image, "/tmp/test.png");
    }

    this->gui_texture->upload(this->ui_image);
    this->ui_needs_update = false;

#endif
}

/* ---------------------------------------------------------------- */
/* -------------------- Offscreen renderer ------------------------ */
/* ---------------------------------------------------------------- */

void
SceneContext::on_offscreen_snapshot (void)
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
        this->print_error("Error saving image", e.what());
    }
}

/* ---------------------------------------------------------------- */

void
SceneContext::on_offscreen_rephoto (void)
{
    mve::View::Ptr active_view = SceneManager::get().get_view();
    this->on_offscreen_rephoto(active_view);
}

/* ---------------------------------------------------------------- */

void
SceneContext::on_offscreen_rephoto (mve::View::Ptr view)
{
    if (view.get() == NULL)
    {
        this->print_error("Error", "No view selected!");
        return;
    }
    std::string source_name
        = this->offscreen_rephoto_source.text().toStdString();
    std::string dest_color_name
        = this->offscreen_rephoto_color_dest.text().toStdString();
    std::string dest_depth_name
        = this->offscreen_rephoto_depth_dest.text().toStdString();

    if (source_name.empty()
        || (dest_color_name.empty() && dest_depth_name.empty()))
    {
        this->print_error("Error", "Invalid embedding names!");
        return;
    }
    mve::MVEFileProxy const* proxy = view->get_proxy(source_name);
    if (proxy == NULL || !proxy->is_image)
    {
        this->print_error("Error", "Embedding not available!");
        return;
    }

    std::cout << "Re-photographing view "
        << view->get_name() << "..." << std::endl;

    /* Backup camera. */
    ogl::Camera camera_backup = this->camera;

    /* Get all parameters and check them. */
    mve::CameraInfo const& camera_info = view->get_camera();
    int const width = proxy->width;
    int const height = proxy->height;
    float const dimension_aspect = static_cast<float>(width) / height;
    float const pixel_aspect = camera_info.paspect;
    float const image_aspect = dimension_aspect * pixel_aspect;
    float const focal_length = view->get_camera().flen;
    float const ppx = view->get_camera().ppoint[0];
    float const ppy = view->get_camera().ppoint[1];

    /* Fill OpenGL view matrix */
    camera_info.fill_world_to_cam(*this->camera.view);

    /* Construct OpenGL projection matrix. */
    math::Matrix4f& proj = this->camera.proj;
    float const znear = 0.001f;
    float const zfar = 1000.0f;
    proj.fill(0.0f);
    proj[0]  = 2.0f * focal_length
        * (image_aspect > 1.0f ? 1.0f : 1.0f / image_aspect);
    proj[2]  = -2.0f * (0.5f - ppx);
    proj[5]  = -2.0f * focal_length
        * (image_aspect > 1.0f ? image_aspect : 1.0f);
    proj[6]  = -2.0f * (ppy - 0.5f);
    proj[10] = -(-zfar - znear) / (zfar - znear);
    proj[11] = -2.0f * zfar * znear / (zfar - znear);
    proj[14] = 1.0f;

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
    this->paint();

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
    this->camera = camera_backup;
    glViewport(0, 0, this->get_width(), this->get_height());
    this->update_gl();
    QApplication::processEvents();

    /* Convert depth buffer to depth map. */
    for (float* ptr = depth->begin(); ptr != depth->end(); ++ptr)
        *ptr = (*ptr == 1.0f)
            ? 0.0f
            : (zfar * znear) / ((znear - zfar) * *ptr + zfar);

    /* Put re-photography into view as embedding */
    view->set_image(dest_color_name, image);
    view->set_image(dest_depth_name, depth);
    view->save_mve_file();
    SceneManager::get().refresh_view();
}

/* ---------------------------------------------------------------- */

void
SceneContext::on_offscreen_rephoto_all (void)
{
    std::string source_embedding_name
        = this->offscreen_rephoto_source.text().toStdString();
    mve::Scene::ViewList& views = this->scene->get_views();
    std::size_t num_rephotographed = 0;
    for (size_t i = 0; i < views.size(); ++i)
    {
        mve::View::Ptr view = views[i];
        if (view.get() == NULL)
            continue;
        if (!view->has_embedding(source_embedding_name))
            continue;
        this->on_offscreen_rephoto(view);
        num_rephotographed += 1;
        if (num_rephotographed % 10 == 0)
            this->scene->cache_cleanup();
    }
    this->scene->cache_cleanup();
    this->print_info("Info", "Re-Photographed "
        + util::string::get(num_rephotographed) + " views!");
}

/* ---------------------------------------------------------------- */

mve::ByteImage::Ptr
SceneContext::get_offscreen_image (void)
{
    /* TODO: Make configurable. */
    float znear = 0.1f;
    float side = 0.05f;

    int w = (int)this->offscreen_width.value();
    int h = (int)this->offscreen_height.value();
    float aspect = (float)w / (float)h;

    /* Backup camera. */
    ogl::Camera camera_backup = this->camera;

    this->camera.width = w;
    this->camera.height = h;
    this->camera.z_near = znear;
    if (w > h)
    {

        this->camera.top = side;
        this->camera.right = side * aspect;
    }
    else
    {
        this->camera.right = side;
        this->camera.top = side / aspect;
    }
    this->camera.update_proj_mat();
    this->camera.update_inv_proj_mat();

    /* Request corresponding GL context. */
    this->request_context();

    /* Set new viewport. */
    glViewport(0, 0, w, h);

    GLuint bf;
    glGenFramebuffers(1, &bf);
    glBindFramebuffer(GL_FRAMEBUFFER, bf);

    GLuint rb[2];
    glGenRenderbuffers(2, rb);

    glBindRenderbuffer(GL_RENDERBUFFER, rb[0]);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB, w, h);
    glBindRenderbuffer(GL_RENDERBUFFER, rb[1]);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h);
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_RENDERBUFFER, rb[0]);
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_RENDERBUFFER, rb[1]);

    this->paint();

    mve::ByteImage::Ptr image = mve::ByteImage::create(w, h, 3);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, image->begin());
    mve::image::flip<uint8_t>(image, mve::image::FLIP_VERTICAL);
    //mve::image::save_file(image, "/tmp/image.png");

    glDeleteRenderbuffers(2, rb);
    glDeleteFramebuffers(1, &bf);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    /* Restore camera. */
    this->camera = camera_backup;
    /* Restore viewport. */
    glViewport(0, 0, this->get_width(), this->get_height());

    return image;
}

/* ---------------------------------------------------------------- */

void
SceneContext::on_offscreen_render_sequence (void)
{
    this->on_offscreen_play_sequence(true);
}

/* ---------------------------------------------------------------- */

void
SceneContext::on_offscreen_play_sequence (bool save)
{
    if (this->offscreen_working)
    {
        this->offscreen_working = false;
        return;
    }

    std::string frame_path = this->offscreen_framedir.get_filename();
    std::string seq_file = this->offscreen_seqfile.get_filename();

    /* Load sequence. */
    CameraSequence sequence;
    try
    { sequence.parse(seq_file); }
    catch (std::exception& e)
    {
        this->print_error("Error reading sequence",
            std::string("Cannot read sequence:\n") + e.what());
        return;
    }

    if (save && frame_path.empty())
    {
        this->print_error("Error saving frames", "No output path specified!");
        return;
    }

    /* Set frame timer for proper animation speed. */
    util::FrameTimer timer;
    timer.set_max_fps(sequence.get_fps());

    this->offscreen_working = true;
    this->offscreen_playbut.setIcon(QIcon(":/images/icon_player_stop.svg"));
    while (this->offscreen_working && sequence.next_frame())
    {
        int frame = sequence.get_frame();

        /* Setup new camera parameters. */
        this->controller.set_camera_params(sequence.get_campos(),
            sequence.get_lookat(), sequence.get_upvec());
        this->update_camera();

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
                this->print_error("Error saving frame!", e.what());
                break;
            }
        }

        /* Render to GL widget. */
        this->update_gl();
        timer.next_frame();

        QApplication::processEvents();
    }
    this->offscreen_working = false;
    this->offscreen_playbut.setIcon(QIcon(":/images/icon_player_play.svg"));
}

/* ---------------------------------------------------------------- */

void
SceneContext::on_offscreen_display_sequence (void)
{
    /* Load sequence. */
    std::string seq_file = this->offscreen_seqfile.get_filename();
    CameraSequence sequence;
    try
    { sequence.parse(seq_file); }
    catch (std::exception& e)
    {
        this->print_error("Error reading sequence",
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

    this->meshlist->add(util::fs::get_file_component(seq_file), mesh, seq_file);
    this->update_gl();
}

/* ---------------------------------------------------------------- */
/* ------------------------ Event handling ------------------------ */
/* ---------------------------------------------------------------- */

void
SceneContext::mouse_event (ogl::MouseEvent const& event)
{
    if (!this->rect_shift_pressed)
    {
        this->ogl::CameraTrackballContext::mouse_event(event);
        return;
    }

    /* Shift is pressed from here on. */


    if (event.button == ogl::MOUSE_BUTTON_LEFT
        && event.type == ogl::MOUSE_EVENT_PRESS)
    {
        this->rect_start_x = event.x;
        this->rect_start_y = event.y;
        this->rect_current_x = event.x;
        this->rect_current_y = event.y;
        return;
    }

    if (event.type == ogl::MOUSE_EVENT_MOVE)
    {
        this->rect_current_x = event.x;
        this->rect_current_y = event.y;
        this->ui_needs_update = true;
        this->update_gl();
        return;
    }

    if (event.button == ogl::MOUSE_BUTTON_LEFT
        && event.type == ogl::MOUSE_EVENT_RELEASE)
    {
        this->rect_shift_pressed = false;
        this->ui_needs_update = true;

        float left = (float)std::min(event.x, this->rect_start_x);
        float right = (float)std::max(event.x, this->rect_start_x);
        float top = (float)std::max(event.y, this->rect_start_y);
        float bottom = (float)std::min(event.y, this->rect_start_y);

        left = 2.0f * left / (float)this->get_width() - 1.0f;
        right = 2.0f * right / (float)this->get_width() - 1.0f;
        top = -2.0f * top / (float)this->get_height() + 1.0f;
        bottom = -2.0f * bottom / (float)this->get_height() + 1.0f;

        this->rect_start_x = this->rect_current_x; // what is this for?
        this->rect_start_y = this->rect_current_y;

        this->screen_debug(left, right, top, bottom);
    }
}

/* ---------------------------------------------------------------- */

void
SceneContext::keyboard_event (ogl::KeyboardEvent const& event)
{
    if (event.keycode == KEY_SHIFT && event.type == ogl::KEYBOARD_EVENT_PRESS)
        this->rect_shift_pressed = true;
    if (event.keycode == KEY_SHIFT && event.type == ogl::KEYBOARD_EVENT_RELEASE)
        this->rect_shift_pressed = false;

    this->ogl::CameraTrackballContext::keyboard_event(event);
}

/* ---------------------------------------------------------------- */

void
SceneContext::screen_debug (float left, float right, float top, float bottom)
{
    if (!this->scene.get())
        return;

    /* Create a text representation of cameras and points in the region. */
    std::stringstream ss;
    ss << "<h2>Selected Cameras</h2>" << std::endl;

    /* Project camera positions to screen space and check if selected. */
    {
        bool found_camera = false;
        mve::Scene::ViewList const& views(this->scene->get_views());
        for (std::size_t i = 0; i < views.size(); ++i)
        {
            if (!views[i].get() || !views[i]->is_camera_valid())
                continue;

            math::Vec4f campos(1.0f);
            views[i]->get_camera().fill_camera_pos(*campos);
            campos = this->camera.view * campos;
            campos = this->camera.proj * campos;
            campos /= campos[3];

            if (campos[0] < left || campos[0] > right
                || campos[1] < top || campos[1] > bottom
                || campos[2] < -1.0 || campos[2] > 1.0)
                continue;

            //std::cout << "ID: " << i << ": " << campos << std::endl;
            found_camera = true;
            ss << "View ID " << i << ", "
                << views[i]->get_name() << "<br/>" << std::endl;
        }

        if (!found_camera)
            //ss << "<i>No cameras selected</i><br/>" << std::endl;
            ss << "<p><i>No cameras selected!</i></p>" << std::endl;
    }

    /* Project bundle points to screen space and check if selected. */
    mve::BundleFile::ConstPtr bundle;
    try { bundle = this->scene->get_bundle(); }
    catch (std::exception& e){ }
    if (bundle.get())
    {
        bool found_points = false;
        ss << "<h2>Selected Bundle Points</h2>";

        mve::BundleFile::FeaturePoints const& points = bundle->get_points();
        mve::Scene::ViewList const& views(this->scene->get_views());
        for (std::size_t i = 0; i < points.size(); ++i)
        {
            math::Vec4f pos(1.0f);
            pos.copy(points[i].pos, 3);
            pos = this->camera.view * pos;
            pos = this->camera.proj * pos;
            pos /= pos[3];

            if (pos[0] < left || pos[0] > right
                || pos[1] < top || pos[1] > bottom
                || pos[2] < -1.0 || pos[2] > 1.0)
                continue;

            found_points = true;
            ss << "Point ID " << i << ", visible in:<br/>" << std::endl;
            for (std::size_t j = 0; j < points[i].refs.size(); ++j)
            {
                mve::View::ConstPtr ref = views[points[i].refs[j].img_id];
                if (!ref.get())
                    continue;
                ss << "&nbsp;&nbsp;View ID ";
                ss << ref->get_id() << ", " << ref->get_name();
                if (!ref->is_camera_valid())
                    ss << " (invalid)";
                else
                    ss << " " <<  points[i].refs[j].error;
                ss << "<br/>" << std::endl;
           }
           ss << "<br/>" << std::endl;
        }

        if (!found_points)
            ss << "<p><i>No points selected!</i></p>" << std::endl;
    }

    this->print_html("Selected Views / Cameras", ss.str());

    //if (!sn.str().empty())
    //    this->print_info("Selected Points", sn.str());
}
