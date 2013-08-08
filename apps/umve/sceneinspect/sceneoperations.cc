#include "guihelpers.h"
#include "scenemanager.h"
#include "sceneoperations.h"

#ifdef USE_BUNDLER

#endif

SceneOperations::SceneOperations (void)
{
    /* Hello world. */
    QLabel* my_test_label = new QLabel("Hello World!");
    QPushButton* update_scene_but = new QPushButton("Update Scene");

    QVBoxLayout* helloworld_layout = new QVBoxLayout();
    helloworld_layout->addWidget(my_test_label);
    helloworld_layout->addWidget(update_scene_but);

#ifdef USE_BUNDLER
    QVBoxLayout* bundler_layout = new QVBoxLayout();

    QPushButton* init_bundler_but = new QPushButton("Initialize Bundler");
    QPushButton* full_ba_but = new QPushButton("Run Full BA");
    QPushButton* run_steps_but = new QPushButton("Run X Steps");
    QPushButton* undist_emb_but = new QPushButton("Create undistorted embeddings");
    QLabel* color_label = new QLabel(" -- ");
    QPushButton* hilight_camera_but = new QPushButton("Hilight Features in Camera: ");
    QPushButton* color_true_but = new QPushButton("Set True Colors");
    QPushButton* color_error_but = new QPushButton("Set Error Colors");
        

    bundler_layout->addWidget(init_bundler_but);
    bundler_layout->addWidget(full_ba_but);
    bundler_layout->addWidget(run_steps_but);
    bundler_layout->addWidget(&this->run_steps);
    bundler_layout->addWidget(undist_emb_but);
    bundler_layout->addWidget(color_label);
    bundler_layout->addWidget(hilight_camera_but);
    bundler_layout->addWidget(&this->hilight_cam);
    bundler_layout->addWidget(color_true_but);
    bundler_layout->addWidget(color_error_but);

#endif

    /* Collapsible headers. */
    QCollapsible* helloworld_header = new QCollapsible
        ("Hello World", get_wrapper(helloworld_layout));
#ifdef USE_BUNDLER
    QCollapsible* bundler_header = new QCollapsible
        ("Bundler", get_wrapper(bundler_layout));
#endif

    helloworld_header->set_collapsed(true);

    /* Manage main layout. */
    QVBoxLayout* main_layout = new QVBoxLayout();
    main_layout->addWidget(helloworld_header);
#ifdef USE_BUNDLER
    main_layout->addWidget(bundler_header);
#endif
    main_layout->addStretch(1);

    this->setLayout(main_layout);

    this->connect(&SceneManager::get(), SIGNAL(scene_selected(mve::Scene::Ptr)),
        this, SLOT(on_scene_selected(mve::Scene::Ptr)));
    this->connect(update_scene_but, SIGNAL(clicked(void)),
        this, SLOT(on_update_scene(void)));

#ifdef USE_BUNDLER
    this->connect(init_bundler_but, SIGNAL(clicked(void)),
                  this, SLOT(on_init_bundler(void)));
    this->connect(full_ba_but, SIGNAL(clicked(void)),
                  this, SLOT(on_full_ba(void)));
    this->connect(undist_emb_but, SIGNAL(clicked(void)),
                  this, SLOT(on_undistort(void)));
    this->connect(run_steps_but, SIGNAL(clicked(void)),
                  this, SLOT(on_run_steps(void)));
    this->connect(hilight_camera_but, SIGNAL(clicked(void)),
                  this, SLOT(on_hilight_camera(void)));
    this->connect(color_true_but, SIGNAL(clicked(void)),
                  this, SLOT(on_true_color(void)));
    this->connect(color_error_but, SIGNAL(clicked(void)),
                  this, SLOT(on_error_color(void)));
#endif

}

/* ---------------------------------------------------------------- */

void
SceneOperations::on_scene_selected (mve::Scene::Ptr scene)
{
    this->scene = scene;
#ifdef USE_BUNDLER
    this->bundler.set_scene(scene);

    this->hilight_cam.setMinimum(0);
    this->run_steps.setMinimum(1);

    if (this->scene != NULL) {
        this->hilight_cam.setMaximum(this->scene->get_bundle()->get_num_cameras());
        this->run_steps.setMaximum(this->scene->get_bundle()->get_num_cameras());
    }
#endif
}

/* ---------------------------------------------------------------- */

void
SceneOperations::on_update_scene (void)
{
    SceneManager::get().refresh_bundle();
    SceneManager::get().refresh_scene();
}

/* ---------------------------------------------------------------- */

#ifdef USE_BUNDLER

void
SceneOperations::on_init_bundler (void)
{
    if (this->scene == NULL)
        return;
    this->bundler.set_scene(this->scene);
    this->bundler.initialize(false, false);
    this->bundler.reconstruct_initial_pair();
    this->bundler.run_full_ba();
    this->on_update_scene();
}

void
SceneOperations::on_full_ba (void)
{
    this->bundler.run_full_ba();
    this->on_update_scene();
}

void
SceneOperations::on_run_steps (void)
{
    std::size_t i = 0;

    std::size_t steps = this->run_steps.value();
    while (i++ < steps)
    {
        this->bundler.reconstruct_new_cam();

        if (this->bundler.is_finished())
            return;

        this->bundler.reconstruct_new_features();
        this->bundler.run_full_ba();
        this->bundler.remove_high_err_features();
        this->on_update_scene();
    }
}

void
SceneOperations::on_error_color (void)
{
    this->bundler.reprojection_color();
    this->on_update_scene();
}

void
SceneOperations::on_true_color (void)
{
    this->bundler.true_color();
    this->on_update_scene();
}

void
SceneOperations::on_hilight_camera (void)
{
    std::size_t cam_num = this->hilight_cam.value();
    this->bundler.hilight_camera_features(cam_num);
    this->on_update_scene();
}


void
SceneOperations::on_undistort (void)
{
    this->bundler.create_undistorted_embeddings();
    this->bundler.save();
    this->on_update_scene();
}


#endif




