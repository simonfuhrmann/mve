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

#include "util/file_system.h"
#include "mve/image.h"
#include "mve/image_tools.h"
#include "sfm/sift.h"
#include "sfm/bundler_common.h"
#include "sfm/bundler_matching.h"
#include "sfm/extract_focal_length.h"

#include "jobqueue.h"
#include "scenemanager.h"
#include "guihelpers.h"
#include "sfm_reconstruct/sfm_controls.h"

SfmControls::SfmControls (GLWidget* gl_widget, QTabWidget* tab_widget)
    : incremental_sfm(this->incremental_opts)
    , tab_widget(tab_widget)
    , clear_color(0, 0, 0)
    , clear_color_cb(new QCheckBox("Background color"))
{
    /* Initialize state and widgets. */
    this->state.gl_widget = gl_widget;
    this->state.ui_needs_redraw = true;

    /* Instanciate addins. */
    this->axis_renderer = new AddinAxisRenderer();
    this->frusta_renderer = new AddinFrustaSfmRenderer();

    /* Register addins. */
    this->addins.push_back(this->axis_renderer);
    this->addins.push_back(this->frusta_renderer);

    /* Create scene rendering form. */
    QFormLayout* rendering_form = new QFormLayout();
    rendering_form->setVerticalSpacing(0);
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
    this->matching_image_embedding = new QLineEdit("original");
    this->matching_feature_embedding = new QLineEdit("original-sift");
    this->matching_exif_embedding = new QLineEdit("exif");
    QPushButton* matching_compute = new QPushButton("Match");
    this->matching_prebundle_file = new QLineEdit("prebundle.bin");
    QPushButton* matching_load = new QPushButton("Load");
    QPushButton* matching_save = new QPushButton("Save");
    QHBoxLayout* matching_buttons = new QHBoxLayout();
    matching_buttons->setSpacing(1);
    matching_buttons->addWidget(matching_load, 1);
    matching_buttons->addWidget(matching_save, 1);

    QFormLayout* matching_form = new QFormLayout();
    matching_form->setVerticalSpacing(0);
    matching_form->addRow(tr("Image:"), this->matching_image_embedding);
    matching_form->addRow(tr("Features:"), this->matching_feature_embedding);
    matching_form->addRow(tr("EXIF:"), this->matching_exif_embedding);
    matching_form->addRow(matching_compute);
    matching_form->addRow(tr("File:"), this->matching_prebundle_file);
    matching_form->addRow(matching_buttons);

    /* Incremental SfM. */
    QPushButton* sfm_recon_init_pair = new QPushButton("Recon Init Pair");
    QPushButton* sfm_recon_next_camera = new QPushButton("Recon Next Camera");
    QPushButton* sfm_recon_all_cameras = new QPushButton("Recon All Cameras");
    QFormLayout* sfm_form = new QFormLayout();
    sfm_form->setVerticalSpacing(0);
    sfm_form->addRow(sfm_recon_init_pair);
    sfm_form->addRow(sfm_recon_next_camera);
    sfm_form->addRow(sfm_recon_all_cameras);

    /* SfM Settings. */
    QFormLayout* settings_form = new QFormLayout();
    // ...

    /* Pack together. */
    QCollapsible* features_header = new QCollapsible("Features",
        get_wrapper(features_form));
    QCollapsible* matching_header = new QCollapsible("Matching",
        get_wrapper(matching_form));
    QCollapsible* sfm_header = new QCollapsible("Incremental SfM",
        get_wrapper(sfm_form));
    QCollapsible* settings_header = new QCollapsible("SfM Settings",
        get_wrapper(settings_form));
    settings_header->set_collapsed(true);

    QVBoxLayout* layout = new QVBoxLayout();
    layout->setSpacing(5);
    layout->addWidget(features_header, 0);
    layout->addWidget(matching_header, 0);
    layout->addWidget(sfm_header, 0);
    layout->addWidget(settings_header, 0);
    layout->addStretch(1);

    /* Signals. */
    this->connect(features_compute, SIGNAL(clicked()),
        this, SLOT(on_features_compute()));
    this->connect(matching_compute, SIGNAL(clicked()),
        this, SLOT(on_matching_compute()));
    this->connect(matching_load, SIGNAL(clicked()),
        this, SLOT(on_prebundle_load()));
    this->connect(matching_save, SIGNAL(clicked()),
        this, SLOT(on_prebundle_save()));
    this->connect(sfm_recon_init_pair, SIGNAL(clicked()),
        this, SLOT(on_recon_init_pair()));
    this->connect(sfm_recon_next_camera, SIGNAL(clicked()),
        this, SLOT(on_recon_next_camera()));
    this->connect(sfm_recon_all_cameras, SIGNAL(clicked()),
        this, SLOT(on_recon_all_cameras()));

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
    /* Clear old SfM state data. */
    this->pairwise_matching.clear();
    this->viewports.clear();

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
    //glPointSize(2.0f);
    //glLineWidth(2.0f);

    this->state.send_uniform(this->camera);
    if (this->state.ui_needs_redraw)
        this->state.clear_ui(ogl::Context::width, ogl::Context::height);

    /* Paint all implementations. */
    for (std::size_t i = 0; i < this->addins.size(); ++i)
        this->addins[i]->paint();

    /* Render SfM points. */
    if (this->sfm_points_renderer == NULL)
        this->create_sfm_points_renderer();
    this->sfm_points_renderer->draw();
}

/* ---------------------------------------------------------------- */

void
SfmControls::create_sfm_points_renderer (void)
{
    mve::TriangleMesh::Ptr mesh = mve::TriangleMesh::create();
    mve::TriangleMesh::VertexList& vertices = mesh->get_vertices();
    mve::TriangleMesh::ColorList& colors = mesh->get_vertex_colors();

    for (std::size_t i = 0; i < this->tracks.size(); ++i)
    {
        if (!this->tracks[i].is_valid())
            continue;

        vertices.push_back(this->tracks[i].pos);
        math::Vec3f color = this->tracks[i].color;
        color /= 255.0f;
        colors.push_back(math::Vec4f(color[0], color[1], color[2], 1.0f));
    }
    this->sfm_points_renderer = ogl::MeshRenderer::create(mesh);
    this->sfm_points_renderer->set_shader(this->state.wireframe_shader);
    this->sfm_points_renderer->set_primitive(GL_POINTS);
}

void
SfmControls::update_frusta_renderer (void)
{

    std::vector<sfm::CameraPose> const& poses = this->incremental_sfm.get_cameras();
    if (this->viewports.size() != poses.size())
        return;

    std::vector<mve::CameraInfo> cameras(poses.size());
    for (std::size_t i = 0; i < cameras.size(); ++i)
    {
        sfm::CameraPose const& pose = poses[i];
        mve::CameraInfo& cam = cameras[i];
        sfm::bundler::Viewport const& viewport = this->viewports[i];
        if (!pose.is_valid())
        {
            cam.flen = 0.0f;
            continue;
        }

        float width = static_cast<float>(viewport.width);
        float height = static_cast<float>(viewport.height);
        float maxdim = std::max(width, height);
        cam.flen = static_cast<float>(pose.get_focal_length());
        cam.flen /= maxdim;
        cam.ppoint[0] = static_cast<float>(pose.K[2]) / width;
        cam.ppoint[1] = static_cast<float>(pose.K[5]) / height;
        std::copy(pose.R.begin(), pose.R.end(), cam.rot);
        std::copy(pose.t.begin(), pose.t.end(), cam.trans);
    }

    this->frusta_renderer->set_cameras(cameras);
}

/* ---------------------------------------------------------------- */

void
SfmControls::initialize_options (void)
{
    /* Matching options. */
    this->matching_opts.matching_opts.descriptor_length = 128; // TODO
    this->matching_opts.ransac_opts.already_normalized = false;
    this->matching_opts.ransac_opts.threshold = 3.0f;
    this->matching_opts.ransac_opts.verbose_output = false;
    //this->matching_opts.min_feature_matches = 24;
    //this->matching_opts.min_matching_inliers = 12;

    /* Initial pair options. */
    this->init_pair_opts.verbose_output = true;
    this->init_pair_opts.max_homography_inliers = 0.5f;
    this->init_pair_opts.homography_opts.max_iterations = 1000;
    this->init_pair_opts.homography_opts.already_normalized = false;
    this->init_pair_opts.homography_opts.threshold = 3.0f;
    this->init_pair_opts.homography_opts.verbose_output = false;

    /* Tracks options. */
    this->tracks_options.verbose_output = true;

    /* Incremental SfM options. */
    this->incremental_opts.fundamental_opts.already_normalized = false;
    this->incremental_opts.fundamental_opts.threshold = 3.0f;
    //this->incremental_opts.fundamental_opts.max_iterations = 1000;
    this->incremental_opts.fundamental_opts.verbose_output = true;
    this->incremental_opts.pose_opts.threshold = 4.0f;
    //this->incremental_opts.pose_opts.max_iterations = 1000;
    this->incremental_opts.pose_opts.verbose_output = true;
    this->incremental_opts.pose_p3p_opts.threshold = 10.0f;
    //this->incremental_opts.pose_p3p_opts.max_iterations = 1000;
    this->incremental_opts.pose_p3p_opts.verbose_output = true;
    //this->incremental_opts.track_error_threshold_factor = 50.0;
    //this->incremental_opts.new_track_error_threshold = 10.0;
    this->incremental_opts.verbose_output = true;
}

/* ---------------------------------------------------------------- */

struct JobSIFT : public JobProgress
{
    mve::View::Ptr view;
    std::string image_name;
    std::string feature_name;
    sfm::Sift::Options sift_options;
    bool auto_save;
    int max_image_size;

    /* Status. */
    QFuture<void> future;
    std::string name;
    std::string message;

    JobSIFT (void) {}
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
        job->message = "Loading image...";
        mve::ByteImage::Ptr image = job->view->get_byte_image(job->image_name);
        if (image == NULL)
        {
            job->message = "No such image.";
            return;
        }

        /* Rescale image until maximum image size is met. */
        job->message = "Rescaling...";
        while (image->width() * image->height() > job->max_image_size)
            image = mve::image::rescale_half_size<uint8_t>(image);

        /* Compute features. */
        job->message = "Features...";
        sfm::Sift feature(job->sift_options);
        feature.set_image(image);
        feature.process();
        sfm::Sift::Descriptors const& descriptors = feature.get_descriptors();

        /* Save features as embedding. */
        job->message = "Finishing...";
        mve::ByteImage::Ptr descr_image = sfm::bundler::descriptors_to_embedding
            (descriptors, image->width(), image->height());
        job->view->set_data(job->feature_name, descr_image);
        if (job->auto_save)
            job->view->save_mve_file();
        job->view->cache_cleanup();

        std::stringstream ss;
        ss << descriptors.size() << " features, done.";
        job->message = ss.str();
    }
}

void
SfmControls::on_features_compute (void)
{
    mve::Scene::Ptr scene = SceneManager::get().get_scene();
    if (scene == NULL)
    {
        QMessageBox::information(this->tab_widget, "Error computing features",
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

        JobSIFT* job = new JobSIFT();
        job->name = view->get_name();
        job->message = "Waiting...";
        job->view = view;
        job->image_name = image_name;
        job->feature_name = feature_name;
        job->sift_options = sfm::Sift::Options();
        job->max_image_size = max_image_size;
        job->auto_save = false;
        job->future = QtConcurrent::run(&run_sift_job, job);
        JobQueue::get()->add_job(job);
   }
}

/* ---------------------------------------------------------------- */

struct JobMatching : public JobProgress
{
    /* Status. */
    QFuture<void> future;
    std::string name;
    std::string message;

    /* Members. */
    mve::Scene::Ptr scene;
    std::string feature_name;
    std::string image_name;
    std::string exif_name;
    sfm::bundler::ViewportList* viewports;
    sfm::bundler::Matching::Options options;
    sfm::bundler::Matching::Progress progress;
    sfm::bundler::PairwiseMatching* result;
    bool canceled;

    JobMatching (void) : viewports(NULL), result(NULL), canceled(false) {}
    char const* get_name (void) { return name.c_str(); }
    bool is_completed (void) { return future.isFinished(); }
    bool has_progress (void) { return true; }
    float get_progress (void) { return (float)progress.num_done / (float)progress.num_total; }
    char const* get_message (void) { return message.c_str(); }
    void cancel_job (void) { }
};

namespace
{
    float
    get_focal_length (mve::ByteImage::ConstPtr exif_data)
    {
        if (exif_data == NULL)
        {
            mve::image::ExifInfo exif;
            sfm::FocalLengthEstimate estimate = sfm::extract_focal_length(exif);
            return estimate.first;
        }

        mve::image::ExifInfo exif = mve::image::exif_extract
            (exif_data->get_byte_pointer(), exif_data->get_byte_size(), false);
        sfm::FocalLengthEstimate estimate = sfm::extract_focal_length(exif);
        return estimate.first;
    }

    void
    run_matching_job (JobMatching* job)
    {
        /* Initialize the viewports. */
        job->message = "Initializing...";
        mve::Scene::ViewList& views(job->scene->get_views());
        job->viewports->clear();
        job->viewports->resize(views.size());
        for (std::size_t i = 0; i < views.size(); ++i)
        {
            mve::View::Ptr view = views[i];
            if (view == NULL)
                continue;
            mve::ByteImage::Ptr descr_data = view->get_data(job->feature_name);
            if (descr_data == NULL)
                continue;
            mve::ByteImage::Ptr image = view->get_byte_image(job->image_name);
            if (image == NULL)
                continue;

            sfm::bundler::Viewport& viewport = job->viewports->at(i);
            sfm::Sift::Descriptors descriptors;
            sfm::bundler::embedding_to_descriptors(descr_data, &descriptors,
                &viewport.width, &viewport.height);

            std::cout << "Initializing " << descriptors.size()
                << " descriptors for view " << i
                << " (" << viewport.width << "x" << viewport.height
                << ")..." << std::endl;

            while (image->width() > viewport.width
                && image->height() > viewport.height)
                image = mve::image::rescale_half_size<uint8_t>(image);
            if (image->width() != viewport.width
                || image->height() != viewport.height)
                continue;

            viewport.descr_data.allocate(descriptors.size() * 128);
            viewport.positions.resize(descriptors.size());
            viewport.colors.resize(descriptors.size());

            float* ptr = viewport.descr_data.begin();
            for (std::size_t i = 0; i < descriptors.size(); ++i, ptr += 128)
            {
                sfm::Sift::Descriptor const& d = descriptors[i];
                std::copy(d.data.begin(), d.data.end(), ptr);
                viewport.positions[i] = math::Vec2f(d.x, d.y);
                image->linear_at(d.x, d.y, viewport.colors[i].begin());
            }

            /* Estimate focal length for view. */
            mve::ByteImage::Ptr exif = view->get_byte_image(job->exif_name);
            viewport.focal_length = get_focal_length(exif);

            /* Cleanup. */
            descr_data.reset();
            image.reset();
            exif.reset();
            view->cache_cleanup();
        }

        job->message = "Matching...";
        sfm::bundler::Matching matching(job->options, &job->progress);
        matching.compute(*job->viewports, job->result);
        job->message = "Done.";
    }
}

void
SfmControls::on_matching_compute (void)
{
    if (this->state.scene == NULL)
    {
        QMessageBox::information(this->tab_widget,
            "Error computing features", "No scene is loaded.");
        return;
    }

    this->initialize_options();

    /* Prepare variables for feature computation. */
    std::string feature_name = this->matching_feature_embedding->text().toStdString();
    std::string image_name = this->matching_image_embedding->text().toStdString();
    std::string exif_name = this->matching_exif_embedding->text().toStdString();

    JobMatching* job = new JobMatching();
    job->name = "SfM Matching";
    job->message = "Waiting...";
    job->scene = this->state.scene;
    job->feature_name = feature_name;
    job->image_name = image_name;
    job->exif_name = exif_name;
    job->options = this->matching_opts;
    job->result = &this->pairwise_matching;
    job->viewports = &this->viewports;
    job->progress.num_done = 0;
    job->progress.num_total = 1;
    job->future = QtConcurrent::run(&run_matching_job, job);
    JobQueue::get()->add_job(job);
}

void
SfmControls::on_prebundle_load (void)
{
    if (this->state.scene == NULL)
    {
        QMessageBox::critical(this->tab_widget,
            "Error loading pre-bundle", "No scene is loaded.");
        return;
    }

    std::string filename = this->matching_prebundle_file->text().toStdString();
    std::string path = util::fs::join_path(this->state.scene->get_path(), filename);
    std::cout << "Loading pre-bundle: " << path << " ..." << std::endl;

    try
    {
        sfm::bundler::load_prebundle_from_file(path,
            &this->viewports, &this->pairwise_matching);
    }
    catch (std::exception& e)
    {
        QMessageBox::critical(this->tab_widget,
            "Error loading pre-bundle", e.what());
        return;
    }

    QMessageBox::information(this->tab_widget,
        "Success", "Pre-bundle loaded.");
}

void
SfmControls::on_prebundle_save (void)
{
    if (this->state.scene == NULL)
    {
        QMessageBox::information(this->tab_widget,
            "Error saving pre-bundle", "No scene is loaded.");
        return;
    }

    if (this->viewports.empty() || this->pairwise_matching.empty())
    {
        QMessageBox::information(this->tab_widget,
            "Error saving pre-bundle", "Missing viewport or matching data.");
        return;
    }

    std::string filename = this->matching_prebundle_file->text().toStdString();
    std::string path = util::fs::join_path(this->state.scene->get_path(), filename);
    std::cout << "Saving pre-bundle: " << path << " ..." << std::endl;

    try
    {
        sfm::bundler::save_prebundle_to_file(this->viewports,
            this->pairwise_matching, path);
    }
    catch (std::exception& e)
    {
        QMessageBox::critical(this->tab_widget,
            "Error saving pre-bundle", e.what());
        return;
    }

    QMessageBox::information(this->tab_widget,
        "Success", "Pre-bundle saved.");
}

/* ---------------------------------------------------------------- */

void
SfmControls::on_recon_init_pair (void)
{
    if (this->viewports.empty() || this->pairwise_matching.empty())
    {
        QMessageBox::information(this->tab_widget,
            "SfM Error", "Viewports or matching not initialized.");
        return;
    }

    this->initialize_options();

    /* Find initial pair. */
    std::cout << "Searching for initial pair..." << std::endl;
    sfm::bundler::InitialPair init_pair(this->init_pair_opts);
    init_pair.compute(this->viewports, this->pairwise_matching,
        &this->init_pair_result);

    if (this->init_pair_result.view_1_id < 0
        || this->init_pair_result.view_2_id < 0)
    {
        QMessageBox::critical(this->tab_widget,
            "SfM Error", "Error finding initial pair.");
        return;
    }

    std::cout << "Using views " << init_pair_result.view_1_id
        << " and " << init_pair_result.view_2_id
        << " as initial pair." << std::endl;

    /* Compute connected feature components, i.e. feature tracks. */
    std::cout << "Computing feature tracks..." << std::endl;
    sfm::bundler::Tracks bundler_tracks(this->tracks_options);
    bundler_tracks.compute(this->pairwise_matching,
        &this->viewports, &this->tracks);
    std::cout << "Created a total of " << this->tracks.size()
        << " tracks." << std::endl;

    /* Clear pairwise matching to save memeory. */
    pairwise_matching.clear();

    /* Initialize incremental SfM. */
    this->incremental_sfm = sfm::bundler::Incremental(this->incremental_opts);
    this->incremental_sfm.initialize(&viewports, &tracks);

    /* Reconstruct pose for the initial pair. */
    std::cout << "Computing pose for initial pair..." << std::endl;
    this->incremental_sfm.reconstruct_initial_pair
        (this->init_pair_result.view_1_id, this->init_pair_result.view_2_id);

    /* Reconstruct track positions with the intial pair. */
    this->incremental_sfm.triangulate_new_tracks();

    /* Remove tracks with large errors. */
    this->incremental_sfm.delete_large_error_tracks();

    /* Run bundle adjustment. */
    std::cout << "Running full bundle adjustment..." << std::endl;
    this->incremental_sfm.bundle_adjustment_full();

    /* Redraw scene. */
    this->sfm_points_renderer.reset();
    this->update_frusta_renderer();
    this->state.repaint();
}

void
SfmControls::on_recon_next_camera (void)
{
    if (!this->incremental_sfm.is_initialized())
    {
        QMessageBox::critical(this->tab_widget,
            "SfM Error", "Incremental SfM not initialized.");
        return;
    }

    this->initialize_options();

    std::vector<int> next_views;
    this->incremental_sfm.find_next_views(&next_views);
    int next_view_id = -1;
    for (std::size_t i = 0; i < next_views.size(); ++i)
    {
        std::cout << std::endl;
        std::cout << "Adding next view ID " << next_views[i] << "..." << std::endl;
        if (this->incremental_sfm.reconstruct_next_view(next_views[i]))
        {
            next_view_id = next_views[i];
            break;
        }
    }

    if (next_view_id < 0)
    {
        std::cout << "No more views to reconstruct." << std::endl;
        QMessageBox::information(this->tab_widget,
            "SfM", "No more views to reconstruct.");
        return;
    }

    this->incremental_sfm.bundle_adjustment_single_cam(next_view_id);
    this->incremental_sfm.triangulate_new_tracks();
    this->incremental_sfm.delete_large_error_tracks();
    this->incremental_sfm.bundle_adjustment_full();

    /* Redraw scene. */
    this->sfm_points_renderer.reset();
    this->update_frusta_renderer();
    this->state.repaint();
}

void
SfmControls::on_recon_all_cameras (void)
{
    QMessageBox::information(this->tab_widget,
        "Notice", "This is not implemented yet.");
}
