#include "guihelpers.h"
#include "scenemanager.h"
#include "sceneoperations.h"

SceneOperations::SceneOperations (void)
{
    /* Hello world. */
    QLabel* my_test_label = new QLabel("Hello World!");
    QPushButton* update_scene_but = new QPushButton("Update Scene");

    QVBoxLayout* helloworld_layout = new QVBoxLayout();
    helloworld_layout->addWidget(my_test_label);
    helloworld_layout->addWidget(update_scene_but);

    /* Collapsible headers. */
    QCollapsible* helloworld_header = new QCollapsible
        ("Hello World", get_wrapper(helloworld_layout));
    helloworld_header->set_collapsed(true);

    /* Manage main layout. */
    QVBoxLayout* main_layout = new QVBoxLayout();
    main_layout->addWidget(helloworld_header);
    main_layout->addStretch(1);

    this->setLayout(main_layout);

    this->connect(&SceneManager::get(), SIGNAL(scene_selected(mve::Scene::Ptr)),
        this, SLOT(on_scene_selected(mve::Scene::Ptr)));
    this->connect(update_scene_but, SIGNAL(clicked(void)),
        this, SLOT(on_update_scene(void)));
}

/* ---------------------------------------------------------------- */

void
SceneOperations::on_scene_selected (mve::Scene::Ptr scene)
{
    this->scene = scene;
}

/* ---------------------------------------------------------------- */

void
SceneOperations::on_update_scene (void)
{
    SceneManager::get().refresh_bundle();
    SceneManager::get().refresh_scene();
}

/* ---------------------------------------------------------------- */
