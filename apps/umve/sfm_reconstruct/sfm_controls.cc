#include <sstream>
#include <iostream>
#include <limits>

#include <QBoxLayout>
#include <QFuture>
#include <QCoreApplication>
#include <QProgressDialog>
#include <QColorDialog>
#include <QFormLayout>
#if QT_VERSION >= 0x050000
#include <QtConcurrent/QtConcurrentRun>
#else
#include <QtConcurrentRun>
#endif

#include "mve/image.h"
#include "mve/image_tools.h"
#include "sfm/sift.h"
#include "sfm/bundler_common.h"

#include "jobqueue.h"
#include "scenemanager.h"
#include "guihelpers.h"
#include "sfm_reconstruct/sfm_controls.h"

SfmControls::SfmControls (GLWidget* gl_widget, QTabWidget* tab_widget)
    : tab_widget(tab_widget)
    , clear_color(0, 0, 0)
    , clear_color_cb(new QCheckBox("Background color"))
{
    /* Initialize state and widgets. */
    this->state.gl_widget = gl_widget;
    this->state.ui_needs_redraw = true;

    /* Instanciate addins. */
    this->axis_renderer = new AddinAxisRenderer();
    this->sfm_renderer = new AddinSfmRenderer();
    this->frusta_renderer = new AddinFrustaRenderer();
    this->selection = new AddinSelection();
    this->selection->set_scene_camera(&this->camera);

    /* Register addins. */
    this->addins.push_back(this->axis_renderer);
    this->addins.push_back(this->sfm_renderer);
    this->addins.push_back(this->frusta_renderer);
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

    /* Create the rendering tab. */
    QVBoxLayout* rendering_layout = new QVBoxLayout();
    rendering_layout->setSpacing(5);
    rendering_layout->addWidget(rendering_header, 0);
    rendering_layout->addWidget(frusta_header, 0);
    rendering_layout->addStretch(1);

    /* Create the SfM tab. */
    QVBoxLayout* sfm_layout = this->create_sfm_layout();

    /* Setup tab widget. */
    this->tab_widget->addTab(get_wrapper(sfm_layout, 5), "SfM");
    this->tab_widget->addTab(get_wrapper(rendering_layout, 5), "Rendering");

    /* Connect signals. */
    this->connect(this->clear_color_cb, SIGNAL(clicked()),
        this, SLOT(on_set_clear_color()));

    /* Finalize UI. */
    this->apply_clear_color();
}

/* ---------------------------------------------------------------- */

QVBoxLayout*
SfmControls::create_sfm_layout (void)
{
    /* Features. */
    this->features_image_embedding = new QLineEdit("original");
    this->features_feature_embedding = new QLineEdit("original-sift");
    this->features_max_pixels = new QSpinBox();
    this->features_max_pixels->setRange(0, std::numeric_limits<int>::max());
    this->features_max_pixels->setValue(5000000);

    QPushButton* features_compute = new QPushButton(tr("Compute missing features"));
    QFormLayout* features_form = new QFormLayout();
    features_form->setVerticalSpacing(0);
    features_form->addRow(tr("Image:"), this->features_image_embedding);
    features_form->addRow(tr("Features:"), this->features_feature_embedding);
    features_form->addRow(tr("Max Pixels:"), this->features_max_pixels);
    features_form->addRow(features_compute);

    /* Matching. */
    this->matching_feature_embedding = new QLineEdit("original-sift");
    QPushButton* matching_recompute = new QPushButton("Match");
    this->matching_output_file = new QLineEdit("matching.bin");
    QPushButton* matching_load = new QPushButton("Load");
    QPushButton* matching_save = new QPushButton("Save");
    QHBoxLayout* matching_buttons = new QHBoxLayout();
    matching_buttons->setSpacing(1);
    matching_buttons->addWidget(matching_load, 1);
    matching_buttons->addWidget(matching_save,1 );

    QFormLayout* matching_form = new QFormLayout();
    matching_form->setVerticalSpacing(0);
    matching_form->addRow(tr("Features:"), this->matching_feature_embedding);
    matching_form->addRow(matching_recompute);
    matching_form->addRow(tr("File:"), this->matching_output_file);
    matching_form->addRow(matching_buttons);

    /* Pack together. */
    QCollapsible* features_header = new QCollapsible("Features",
        get_wrapper(features_form));
    QCollapsible* matching_header = new QCollapsible("Matching",
        get_wrapper(matching_form));

    QVBoxLayout* layout = new QVBoxLayout();
    layout->setSpacing(5);
    layout->addWidget(features_header, 0);
    layout->addWidget(matching_header, 0);
    layout->addStretch(1);

    /* Signals. */
    this->connect(features_compute, SIGNAL(clicked()),
        this, SLOT(on_features_compute()));

    return layout;
}

/* ---------------------------------------------------------------- */

bool
SfmControls::keyboard_event(const ogl::KeyboardEvent &event)
{
    for (std::size_t i = 0; i < this->addins.size(); ++i)
        if (this->addins[i]->keyboard_event(event))
            return true;

    return ogl::CameraTrackballContext::keyboard_event(event);
}

bool
SfmControls::mouse_event (const ogl::MouseEvent &event)
{
    for (std::size_t i = 0; i < this->addins.size(); ++i)
        if (this->addins[i]->mouse_event(event))
            return true;

    return ogl::CameraTrackballContext::mouse_event(event);
}

/* ---------------------------------------------------------------- */

void
SfmControls::set_scene (mve::Scene::Ptr scene)
{
    this->state.scene = scene;
    this->state.repaint();
}

void
SfmControls::set_view (mve::View::Ptr view)
{
    this->state.view = view;
    this->state.repaint();
}

/* ---------------------------------------------------------------- */

void
SfmControls::apply_clear_color (void)
{
    QPalette pal;
    pal.setColor(QPalette::Base, this->clear_color);
    this->clear_color_cb->setPalette(pal);
}

void
SfmControls::on_set_clear_color (void)
{
    this->clear_color_cb->setChecked(false);
    QColor newcol = QColorDialog::getColor(this->clear_color, this);
    if (!newcol.isValid())
        return;

    this->clear_color = newcol;
    this->apply_clear_color();
    this->state.gl_widget->repaint();
}

/* ---------------------------------------------------------------- */

void
SfmControls::load_shaders (void)
{
    this->state.load_shaders();
}

/* ---------------------------------------------------------------- */

void
SfmControls::init_impl (void)
{
#ifdef _WIN32
    /* Initialize GLEW. */
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        std::cout << "Error initializing GLEW: " << glewGetErrorString(err)
            << std::endl;
        std::exit(1);
    }
#endif

    /* Load shaders. */
    this->state.load_shaders();
    this->state.init_ui();

    /* Init addins. */
    for (std::size_t i = 0; i < this->addins.size(); ++i)
    {
        this->addins[i]->set_state(&this->state);
        this->addins[i]->init();
    }
}

void
SfmControls::resize_impl (int old_width, int old_height)
{
    this->ogl::CameraTrackballContext::resize_impl(old_width, old_height);
    for (std::size_t i = 0; i < this->addins.size(); ++i)
        this->addins[i]->resize(this->ogl::Context::width, this->ogl::Context::height);

    this->state.ui_needs_redraw = true;
}

void
SfmControls::paint_impl (void)
{
    /* Set clear color and depth. */
    glClearColor(this->clear_color.red() / 255.0f,
        this->clear_color.green() / 255.0f,
        this->clear_color.blue() / 255.0f,
        this->clear_color.alpha() / 255.0f);

    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // TODO: Make configurable through GUI
    glPointSize(2.0f);
    //glLineWidth(2.0f);

    this->state.send_uniform(this->camera);
    if (this->state.ui_needs_redraw)
        this->state.clear_ui(ogl::Context::width, ogl::Context::height);

    /* Paint all implementations. */
    for (std::size_t i = 0; i < this->addins.size(); ++i)
        this->addins[i]->paint();

    /* Draw UI. TODO: Move to state? */
    if (this->state.ui_needs_redraw)
        this->state.gui_texture->upload(this->state.ui_image);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    this->state.gui_texture->bind();
    this->state.texture_shader->bind();
    this->state.gui_renderer->draw();
    this->state.ui_needs_redraw = false;
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

/* ---------------------------------------------------------------- */

struct JobSIFT : public JobProgress
{
    mve::View::Ptr view;
    mve::ByteImage::Ptr image;
    std::string feature_name;
    sfm::Sift::Options sift_options;
    bool auto_save;
    int max_image_size;

    /* Status. */
    QFuture<void> future;
    std::string name;
    std::string message;
    bool thread_started;

    JobSIFT (void) : thread_started(false) {}
    char const* get_name (void) { return name.c_str(); }
    bool is_completed (void) { return future.isFinished(); }
    bool has_progress (void) { return false; }
    float get_progress (void) { return 0.0f; }
    char const* get_message (void) { return message.c_str(); }
    void cancel_job (void) { }
};

namespace
{
    void run_sift_job (JobSIFT* job)
    {
        /* Rescale image until maximum image size is met. */
        job->message = "Rescaling...";
        while (job->image->width() * job->image->height() > job->max_image_size)
            job->image = mve::image::rescale_half_size<uint8_t>(job->image);

        /* Compute features. */
        job->message = "Features...";
        sfm::Sift feature(job->sift_options);
        feature.set_image(job->image);
        feature.process();
        sfm::Sift::Descriptors const& descriptors = feature.get_descriptors();

        /* Save features as embedding. */
        job->message = "Finishing...";
        mve::ByteImage::Ptr descr_image = sfm::bundler::descriptors_to_embedding
            (descriptors, job->image->width(), job->image->height());
        job->view->set_data(job->feature_name, descr_image);
        if (job->auto_save)
            job->view->save_mve_file();
        job->view->cache_cleanup();

        job->message = "Done.";
    }
}

void
SfmControls::on_features_compute (void)
{
    mve::Scene::Ptr scene = SceneManager::get().get_scene();
    if (scene == NULL)
    {
        QMessageBox::information(this, "Error computing features",
            "No scene is loaded.");
        return;
    }

    /* Prepare variables for feature computation. */
    mve::Scene::ViewList& views(scene->get_views());
    std::string image_name = this->features_image_embedding->text().toStdString();
    std::string feature_name = this->features_feature_embedding->text().toStdString();
    int max_image_size = this->features_max_pixels->value();

    for (std::size_t i = 0; i < views.size(); ++i)
    {
        mve::View::Ptr view = views[i];
        if (view == NULL)
            continue;
        mve::ByteImage::Ptr img = view->get_byte_image(image_name);
        if (img == NULL)
            continue;

        JobSIFT* job = new JobSIFT();
        job->view = view;
        job->image = img;
        job->feature_name = feature_name;
        job->sift_options = sfm::Sift::Options();
        job->max_image_size = max_image_size;
        job->auto_save = false;
        job->name = view->get_name();
        job->future = QtConcurrent::run(&run_sift_job, job);
        JobQueue::get()->add_job(job);
   }
}
