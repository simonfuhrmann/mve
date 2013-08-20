#include <iostream>
#include <QFileDialog>
#include <QMessageBox>
#include <QToolBar>

#include "guihelpers.h"
#include "scenemanager.h"
#include "scenecontext.h"
#include "sceneinspect.h"

#ifdef USE_MVPS_CONTEXT
#include "mvps/qtgui/mvpscontext.h"
#endif

SceneInspect::SceneInspect (QWidget* parent)
    : QWidget(parent)
{
    /* Create GL widget. */
    this->glw = new GLWidget();

    /* Create toolbar, add actions. */
    this->create_actions();
    QToolBar* toolbar = new QToolBar("Mesh tools");
    toolbar->addAction(this->action_open_mesh);
    toolbar->addAction(this->action_reload_shaders);
    toolbar->addSeparator();
    toolbar->addAction(this->action_save_screenshot);
    toolbar->addWidget(get_expander());
    toolbar->addAction(this->action_show_details);

    /* Create details notebook. */
    this->scene_details = new QTabWidget();
    this->scene_details->setTabPosition(QTabWidget::East);
    //this->scene_details->hide();

    /* Construct contexts and add to pane. */
#ifndef USE_MVPS_CONTEXT
    this->contexts.push_back(new SceneContext());
    this->activate_context(0);
#else
    this->contexts.push_back(new SceneContext());
    this->contexts.push_back(new MVPSContext());
    this->activate_context(1);
#endif

    for (std::size_t i = 0; i < this->contexts.size(); ++i)
    {
        GuiContext* ctx(this->contexts[i]);
        ctx->set_gl_widget(this->glw);
        if (ctx->get_gui_name() == 0)
            continue;
        QString guiname(ctx->get_gui_name());
        this->scene_details->addTab(ctx, guiname);
    }

    /* Add scene operations tab. */
    this->scene_operations = new SceneOperations();
    this->scene_details->addTab(this->scene_operations, "Operations");

    /* Connect some signal. */
    this->connect(this->scene_details, SIGNAL(currentChanged(int)),
        this, SLOT(on_tab_changed(int)));
    this->connect(&SceneManager::get(), SIGNAL(scene_selected(mve::Scene::Ptr)),
        this, SLOT(on_scene_selected(mve::Scene::Ptr)));
    this->connect(&SceneManager::get(), SIGNAL(view_selected(mve::View::Ptr)),
        this, SLOT(on_view_selected(mve::View::Ptr)));

    /* Pack everything together. */
    QVBoxLayout* vbox = new QVBoxLayout();
    vbox->addWidget(toolbar);
    vbox->addWidget(this->glw);

    QHBoxLayout* main_layout = new QHBoxLayout(this);
    main_layout->addLayout(vbox, 1);
    main_layout->addWidget(this->scene_details);
}

/* ---------------------------------------------------------------- */

void
SceneInspect::create_actions (void)
{
    this->action_open_mesh = new QAction(QIcon(":/images/icon_open_file.svg"),
        tr("Open mesh"), this);
    this->connect(this->action_open_mesh, SIGNAL(triggered()),
        this, SLOT(on_open_mesh()));

    this->action_reload_shaders = new QAction(QIcon(":/images/icon_revert.svg"),
        tr("Reload shaders"), this);
    this->connect(this->action_reload_shaders, SIGNAL(triggered()),
        this, SLOT(on_reload_shaders()));

    this->action_show_details = new QAction
        (QIcon(":/images/icon_toolbox.svg"), tr("Show &Details"), this);
    this->action_show_details->setCheckable(true);
    this->connect(this->action_show_details, SIGNAL(triggered()),
        this, SLOT(on_details_toggled()));

    this->action_save_screenshot = new QAction(QIcon(":/images/icon_screenshot.svg"),
        tr("Save Screenshot"), this);
    this->connect(this->action_save_screenshot, SIGNAL(triggered()),
        this, SLOT(on_save_screenshot()));
}

/* ---------------------------------------------------------------- */

void
SceneInspect::on_open_mesh (void)
{
    QStringList filenames = QFileDialog::getOpenFileNames(this,
        tr("Open Mesh"), QDir::currentPath());

    for (QStringList::iterator it = filenames.begin();
         it != filenames.end(); ++it)
    {
        this->load_file(it->toStdString());
    }
}

/* ---------------------------------------------------------------- */

void
SceneInspect::on_details_toggled (void)
{
    bool show = this->action_show_details->isChecked();
    this->scene_details->setVisible(show);
}

/* ---------------------------------------------------------------- */

void
SceneInspect::on_tab_changed (int id)
{
    //ogl::Context* context = dynamic_cast<ogl::Context*>(this->contexts[id]);
    //std::cout << "Tab changed to ID " << id << ", context " << context << std::endl;

    /* Checke if given ID is an actual context. */
    if ((std::size_t)id < this->contexts.size())
    {
        this->activate_context(id);
        this->glw->repaint();
    }
}

/* ---------------------------------------------------------------- */

void
SceneInspect::load_file (std::string const& filename)
{
    if (this->current_context >= this->contexts.size())
        return;
    this->contexts[this->current_context]->load_file(filename);
    this->glw->repaint();
}

/* ---------------------------------------------------------------- */

void
SceneInspect::activate_context (std::size_t id)
{
    this->current_context = id;
    GuiContext* cc(this->contexts[id]);
    this->glw->set_context(dynamic_cast<ogl::Context*>(cc));
}

/* ---------------------------------------------------------------- */

void
SceneInspect::on_reload_shaders (void)
{
    std::cout << "Reloading shaders..." << std::endl;
    for (std::size_t i = 0; i < this->contexts.size(); ++i)
        this->contexts[i]->reload_shaders();
    this->glw->repaint();
}

/* ---------------------------------------------------------------- */

void
SceneInspect::on_scene_selected (mve::Scene::Ptr scene)
{
    for (std::size_t i = 0; i < this->contexts.size(); ++i)
        this->contexts[i]->set_scene(scene);
}

/* ---------------------------------------------------------------- */

void
SceneInspect::on_view_selected (mve::View::Ptr view)
{
    for (std::size_t i = 0; i < this->contexts.size(); ++i)
        this->contexts[i]->set_view(view);
}

/* ---------------------------------------------------------------- */

void
SceneInspect::on_save_screenshot (void)
{
    QString filename = QFileDialog::getSaveFileName(this,
        "Export Image...");
    if (filename.size() == 0)
        return;

    QImage img = this->glw->grabFrameBuffer(false);
    bool success = img.save(filename);
    if (!success)
        QMessageBox::critical(this, "Cannot save image",
            "Error saving image");

    /* OMG Code */
    //QPixmap pix = this->glw->renderPixmap(100, 100);
    //pix.save("/tmp/test.png");
}

/* ---------------------------------------------------------------- */

void
SceneInspect::reset (void)
{
    for (std::size_t i = 0; i < this->contexts.size(); ++i)
        this->contexts[i]->reset();
}
