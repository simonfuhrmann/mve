#include <iostream>

#include <QFormLayout>
#include <QBoxLayout>
#include <QColorDialog>

#include "util/exception.h"
#include "util/file_system.h"
#include "ogl/render_tools.h"

#include "guihelpers.h"
#include "scene_inspect/addin_manager.h"

AddinManager::AddinManager (GLWidget* gl_widget, QTabWidget* tab_widget)
    : tab_widget(tab_widget)
    , clear_color(0, 0, 0)
    , clear_color_cb(new QCheckBox("Background color"))
{
    /* Initialize state and widgets. */
    this->state.gl_widget = gl_widget;
    this->state.ui_needs_redraw = true;
    this->selected_view_1 = new SelectedView();
    this->selected_view_2 = new SelectedView();

    /* Instanciate addins. */
    this->axis_renderer = new AddinAxisRenderer();
    this->sfm_renderer = new AddinSfmRenderer();
    this->frusta_renderer = new AddinFrustaRenderer();
    this->mesh_renderer = new AddinMeshesRenderer();
    this->dm_triangulate = new AddinDMTriangulate();
    this->dm_triangulate->set_selected_view(this->selected_view_2);
    this->offscreen_renderer = new AddinOffscreenRenderer();
    this->offscreen_renderer->set_scene_camera(&this->camera);
    this->rephotographer = new AddinRephotographer();
    this->rephotographer->set_scene_camera(&this->camera);
    this->aabb_creator = new AddinAABBCreator();
    this->plane_creator = new AddinPlaneCreator();
    this->sphere_creator = new AddinSphereCreator();
    this->selection = new AddinSelection();
    this->selection->set_scene_camera(&this->camera);

    /* Register addins. */
    this->addins.push_back(this->axis_renderer);
    this->addins.push_back(this->sfm_renderer);
    this->addins.push_back(this->frusta_renderer);
    this->addins.push_back(this->mesh_renderer);
    this->addins.push_back(this->dm_triangulate);
    this->addins.push_back(this->offscreen_renderer);
    this->addins.push_back(this->rephotographer);
    this->addins.push_back(this->aabb_creator);
    this->addins.push_back(this->plane_creator);
    this->addins.push_back(this->sphere_creator);
    this->addins.push_back(this->selection);

    /* Create scene rendering form. */
    QFormLayout* rendering_form = new QFormLayout();
    rendering_form->setVerticalSpacing(0);
    rendering_form->addRow(this->sfm_renderer->get_sidebar_widget());
    rendering_form->addRow(this->axis_renderer->get_sidebar_widget());
    rendering_form->addRow(this->clear_color_cb);

    /* Create sidebar headers. */
    QCollapsible* rendering_header = new QCollapsible("Scene Rendering",
        get_wrapper(rendering_form));
    QCollapsible* frusta_header = new QCollapsible("Frusta Rendering",
        this->frusta_renderer->get_sidebar_widget());
    QCollapsible* mesh_header = new QCollapsible("Mesh Rendering",
        this->mesh_renderer->get_sidebar_widget());
    mesh_header->set_collapsible(false);
    QCollapsible* dm_triangulate_header = new QCollapsible("DM Triangulate",
        this->dm_triangulate->get_sidebar_widget());
    QCollapsible* offscreen_header = new QCollapsible("Offscreen Rendering",
        this->offscreen_renderer->get_sidebar_widget());
    offscreen_header->set_collapsed(true);
    QCollapsible* rephotographer_header = new QCollapsible("Rephotographer",
        this->rephotographer->get_sidebar_widget());
    rephotographer_header->set_collapsed(true);
    QCollapsible* aabb_creator_header = new QCollapsible("AABB Creator",
        this->aabb_creator->get_sidebar_widget());
    aabb_creator_header->set_collapsed(true);
    QCollapsible* plane_creator_header = new QCollapsible("Plane Creator",
        this->plane_creator->get_sidebar_widget());
    plane_creator_header->set_collapsed(true);
    QCollapsible* sphere_creator_header = new QCollapsible("Sphere Creator",
        this->sphere_creator->get_sidebar_widget());
    sphere_creator_header->set_collapsed(true);

    /* Create the rendering tab. */
    QVBoxLayout* rendering_layout = new QVBoxLayout();
    rendering_layout->setSpacing(5);
    rendering_layout->addWidget(this->selected_view_1, 0);
    rendering_layout->addWidget(rendering_header, 0);
    rendering_layout->addWidget(frusta_header, 0);
    rendering_layout->addWidget(mesh_header, 1);

    /* Create the operations tab. */
    QVBoxLayout* operations_layout = new QVBoxLayout();
    operations_layout->setSpacing(5);
    operations_layout->addWidget(this->selected_view_2, 0);
    operations_layout->addWidget(dm_triangulate_header, 0);
    operations_layout->addWidget(offscreen_header, 0);
    operations_layout->addWidget(rephotographer_header, 0);
    operations_layout->addWidget(aabb_creator_header, 0);
    operations_layout->addWidget(plane_creator_header, 0);
    operations_layout->addWidget(sphere_creator_header, 0);
    operations_layout->addStretch(1);

    /* Setup tab widget. */
    this->tab_widget->addTab(get_wrapper(rendering_layout, 5), "Rendering");
    this->tab_widget->addTab(get_wrapper(operations_layout, 5), "Operations");

    /* Connect signals. */
    this->connect(this->clear_color_cb, SIGNAL(clicked()),
        this, SLOT(on_set_clear_color()));

    /* Finalize UI. */
    this->apply_clear_color();
}

/* ---------------------------------------------------------------- */

bool
AddinManager::keyboard_event(const ogl::KeyboardEvent &event)
{
    for (std::size_t i = 0; i < this->addins.size(); ++i)
        if (this->addins[i]->keyboard_event(event))
            return true;

    return ogl::CameraTrackballContext::keyboard_event(event);
}

bool
AddinManager::mouse_event (const ogl::MouseEvent &event)
{
    for (std::size_t i = 0; i < this->addins.size(); ++i)
        if (this->addins[i]->mouse_event(event))
            return true;

    return ogl::CameraTrackballContext::mouse_event(event);
}

/* ---------------------------------------------------------------- */

void
AddinManager::load_file (std::string const& filename)
{
    this->mesh_renderer->load_mesh(filename);
}

void
AddinManager::set_scene (mve::Scene::Ptr scene)
{
    this->state.scene = scene;
    this->state.gl_widget->repaint();
}

void
AddinManager::set_view (mve::View::Ptr view)
{
    this->state.view = view;
    this->selected_view_1->set_view(this->state.view);
    this->selected_view_2->set_view(this->state.view);
    this->state.gl_widget->repaint();
}

void
AddinManager::reset_scene (void)
{
    this->state.scene = NULL;
    this->state.view = NULL;
    this->selected_view_1->set_view(mve::View::Ptr());
    this->selected_view_2->set_view(mve::View::Ptr());
    this->state.gl_widget->repaint();
}

static void
load_shaders_from_resources(ogl::ShaderProgram::Ptr prog, QString base)
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

void
AddinManager::load_shaders (void)
{
    std::string binary_folder =
        util::fs::get_path_component(util::fs::get_binary_path());

    std::string home_local = std::string(getenv("HOME"));

    std::vector<std::string> shader_path;
    shader_path.push_back("/usr/share/umve/shader/");
    shader_path.push_back("/usr/local/share/umve/shader/");
    shader_path.push_back(home_local + "/.local/share/umve/shader");
    shader_path.push_back(binary_folder + "/shader/");

    /*
     * Shaders loaded later override those loaded earlier, so try
     * Qt Resources first and files afterwards.
     */

    load_shaders_from_resources(this->state.surface_shader, ":/shader/surface_120");
    load_shaders_from_resources(this->state.wireframe_shader, ":/shader/wireframe_120");
    load_shaders_from_resources(this->state.texture_shader, ":/shader/texture_120");

    for (std::size_t i = 0; i < shader_path.size(); ++i)
    {
        try {
            this->state.surface_shader->try_load_all(shader_path[i] + "surface_120", false);
            this->state.wireframe_shader->try_load_all(shader_path[i] + "wireframe_120", false);
            this->state.texture_shader->try_load_all(shader_path[i] + "texture_120", false);
        } catch (util::Exception &e) {
            std::cout << "Skipping shaders from " << shader_path[i] << ":" << std::endl;
            std::cout << e.what() << std::endl;
        }
    }
}

/* ---------------------------------------------------------------- */

void
AddinManager::init_impl (void)
{
    std::cout << "Initializing OpenGL..." << std::endl;

    /* Initialize GLEW. */
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK)
        std::cout << "Error initializing GLEW: " << glewGetErrorString(err)
            << std::endl;

    /* Load shaders. */
    this->state.surface_shader = ogl::ShaderProgram::create();
    this->state.wireframe_shader = ogl::ShaderProgram::create();
    this->state.texture_shader = ogl::ShaderProgram::create();
    this->load_shaders();
    this->state.gui_renderer = ogl::create_fullscreen_quad(this->state.texture_shader);
    this->state.gui_texture = ogl::Texture::create();

    for (std::size_t i = 0; i < this->addins.size(); ++i)
    {
        this->addins[i]->set_state(&this->state);
        this->addins[i]->init();
        this->connect(this->addins[i], SIGNAL(mesh_generated(
            std::string const&, mve::TriangleMesh::Ptr)), this,
            SLOT(on_mesh_generated(std::string const&,
            mve::TriangleMesh::Ptr)));
    }
}

void
AddinManager::resize_impl (int old_width, int old_height)
{
    this->ogl::CameraTrackballContext::resize_impl(old_width, old_height);
    for (std::size_t i = 0; i < this->addins.size(); ++i)
        this->addins[i]->resize(this->ogl::Context::width, this->ogl::Context::height);

    this->state.ui_needs_redraw = true;
}

void
AddinManager::paint_impl (void)
{
    /* Set clear color and depth. */
    glClearColor(this->clear_color.red() / 255.0f,
        this->clear_color.green() / 255.0f,
        this->clear_color.blue() / 255.0f,
        this->clear_color.alpha() / 255.0f);

    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // TODO: Make configurable through GUI
    //glPointSize(4.0f);
    //glLineWidth(2.0f);

    /* Setup wireframe shader. */
    this->state.wireframe_shader->bind();
    this->state.wireframe_shader->send_uniform("viewmat", this->camera.view);
    this->state.wireframe_shader->send_uniform("projmat", this->camera.proj);

    /* Setup surface shader. */
    this->state.surface_shader->bind();
    this->state.surface_shader->send_uniform("viewmat", this->camera.view);
    this->state.surface_shader->send_uniform("projmat", this->camera.proj);

    /* Setup texture shader. */
    this->state.texture_shader->bind();
    this->state.texture_shader->send_uniform("viewmat", this->camera.view);
    this->state.texture_shader->send_uniform("projmat", this->camera.proj);

    if (this->state.ui_needs_redraw)
    {
        this->state.ui_image = mve::ByteImage::create(ogl::Context::width,
            ogl::Context::height, 4);
        this->state.ui_image->fill(0);
    }

    /* Paint all implementations. */
    for (std::size_t i = 0; i < this->addins.size(); ++i)
    {
        if (this->state.ui_needs_redraw)
            this->addins[i]->redraw_gui();
        this->addins[i]->paint();
    }

    if (this->state.ui_needs_redraw)
        this->state.gui_texture->upload(this->state.ui_image);

    /* Draw UI. */
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    this->state.gui_texture->bind();
    this->state.texture_shader->bind();
    this->state.gui_renderer->draw();
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    this->state.ui_needs_redraw = false;
}

/* ---------------------------------------------------------------- */

void
AddinManager::apply_clear_color (void)
{
    QPalette pal;
    pal.setColor(QPalette::Base, this->clear_color);
    this->clear_color_cb->setPalette(pal);
}

void
AddinManager::on_set_clear_color (void)
{
    this->clear_color_cb->setChecked(false);
    QColor newcol = QColorDialog::getColor(this->clear_color, this);
    if (!newcol.isValid())
        return;

    this->clear_color = newcol;
    this->apply_clear_color();
    this->state.gl_widget->repaint();
}

void
AddinManager::on_mesh_generated (std::string const& name,
    mve::TriangleMesh::Ptr mesh)
{
    this->mesh_renderer->add_mesh(name, mesh);
}
