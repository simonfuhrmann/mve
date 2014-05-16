#include <iostream>
#include <QBoxLayout>

#include "scenemanager.h"
#include "sfm_reconstruct/sfm_reconstruct.h"

SfmReconstruct::SfmReconstruct (QWidget* parent)
    : MainWindowTab(parent)
{
    /* Create details notebook. */
    this->tab_widget = new QTabWidget();
    this->tab_widget->setTabPosition(QTabWidget::East);

    /* Create GL context. */
    this->gl_widget = new GLWidget();
    this->sfm_controls = new SfmControls(this->gl_widget, this->tab_widget);
    this->gl_widget->set_context(this->sfm_controls);

    this->connect(&SceneManager::get(), SIGNAL(scene_selected(mve::Scene::Ptr)),
        this, SLOT(on_scene_selected(mve::Scene::Ptr)));
    this->connect(&SceneManager::get(), SIGNAL(view_selected(mve::View::Ptr)),
        this, SLOT(on_view_selected(mve::View::Ptr)));
    this->connect(this, SIGNAL(tab_activated()), SLOT(on_tab_activated()));

    QHBoxLayout* main_layout = new QHBoxLayout(this);
    main_layout->addWidget(this->gl_widget, 1);
    main_layout->addWidget(this->tab_widget);
}

void
SfmReconstruct::on_scene_selected (mve::Scene::Ptr scene)
{
    //if (!this->is_tab_active)
    //    return;
    this->sfm_controls->set_scene(scene);
}

void
SfmReconstruct::on_view_selected (mve::View::Ptr view)
{
    //if (!this->is_tab_active)
    //    return;
    this->sfm_controls->set_view(view);
}

QString
SfmReconstruct::get_title (void)
{
    return tr("SfM Reconstruct");
}

void
SfmReconstruct::on_tab_activated (void)
{
}
