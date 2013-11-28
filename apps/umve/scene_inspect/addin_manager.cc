#include <iostream>

#include <QFormLayout>
#include <QBoxLayout>
#include <QColorDialog>

#include "util/file_system.h"

#include "guihelpers.h"
#include "scene_inspect/addin_manager.h"

AddinManager::AddinManager (GLWidget* gl_widget, QTabWidget* tab_widget)
    : tab_widget(tab_widget)
    , clear_color(0, 0, 0)
    , clear_color_cb(new QCheckBox("Background color"))
{
    /* Initialize state and widgets. */
    this->state.gl_widget = gl_widget;
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

    /* Register addins. */
    this->addins.push_back(this->axis_renderer);
    this->addins.push_back(this->sfm_renderer);
    this->addins.push_back(this->frusta_renderer);
    this->addins.push_back(this->mesh_renderer);
    this->addins.push_back(this->dm_triangulate);
    this->addins.push_back(this->offscreen_renderer);
    this->addins.push_back(this->rephotographer);
    this->addins.push_back(this->aabb_creator);

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

void
AddinManager::reload_shaders (void)
{
    std::cout << "Reloading shaders..." << std::endl;
    if (this->state.surface_shader != NULL)
        this->state.surface_shader->reload_all();
    if (this->state.wireframe_shader != NULL)
        this->state.wireframe_shader->reload_all();
    if (this->state.texture_shader != NULL)
        this->state.texture_shader->reload_all();
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
    std::string shader_path = util::fs::get_path_component
        (util::fs::get_binary_path()) + "/shader/";
    this->state.surface_shader = ogl::ShaderProgram::create();
    this->state.wireframe_shader = ogl::ShaderProgram::create();
    this->state.texture_shader = ogl::ShaderProgram::create();
    this->state.surface_shader->load_all(shader_path + "surface_120");
    this->state.wireframe_shader->load_all(shader_path + "wireframe_120");
    this->state.texture_shader->load_all(shader_path + "texture_120");

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

    /* Paint all implementations. */
    for (std::size_t i = 0; i < this->addins.size(); ++i)
        this->addins[i]->paint();
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
