// Keep on top because of Eigen
#include "dmrecon/DMRecon.h"
#include "dmrecon/Settings.h"

#include <iostream>
#include <QFormLayout>
#include <QFuture>
#include <QMessageBox>
#include <QtGlobal>
#if QT_VERSION >= 0x050000
#include <QtConcurrent/QtConcurrentRun>
#else
#include <QtConcurrentRun>
#endif

#include "mve/view.h"
#include "mve/depthmap.h"
#include "mve/bilateral.h"
#include "mve/plyfile.h"

#include "jobqueue.h"
#include "guihelpers.h"
#include "scenemanager.h"
#include "imageoperations.h"

ImageOperationsWidget::ImageOperationsWidget (void)
{
    this->selected_view = new SelectedView();

    const mvs::Settings default_settings;

    /* Depthmap recon layout. */
    this->mvs_color_scale.setText("Enable Color Scale");
    this->mvs_color_scale.setChecked(default_settings.useColorScale);
    this->mvs_write_ply.setText("Write PLY after recon");
    this->mvs_write_ply.setChecked(default_settings.writePlyFile);
    this->mvs_dz_map.setText("Keep dz map");
    this->mvs_dz_map.setChecked(default_settings.keepDzMap);
    this->mvs_conf_map.setText("Keep confidence map");
    this->mvs_conf_map.setChecked(default_settings.keepConfidenceMap);
    this->mvs_auto_save.setText("Save view after recon");
    this->mvs_auto_save.setChecked(false);
    this->mvs_amount_gvs.setValue(default_settings.globalVSMax);
    this->mvs_scale.setValue(default_settings.scale);
    this->mvs_scale.setRange(0, 10);

    QPushButton* dmrecon_but = new QPushButton
        (QIcon(":/images/icon_exec.svg"), "MVS reconstruct (F3)");
    dmrecon_but->setIconSize(QSize(18, 18));
    dmrecon_but->setShortcut(QKeySequence(Qt::Key_F3));
    QPushButton* dmrecon_batch_but = new QPushButton
        (QIcon(":/images/icon_exec.svg"), "Batch reconstruct");
    dmrecon_batch_but->setIconSize(QSize(18, 18));
    QFormLayout* mvs_entry_layout = new QFormLayout();
    mvs_entry_layout->setVerticalSpacing(0);
    mvs_entry_layout->addRow(tr("Neighbors"), &this->mvs_amount_gvs);
    mvs_entry_layout->addRow(tr("Scale"), &this->mvs_scale);
    mvs_entry_layout->addRow(tr("Image"), &this->mvs_color_image);
    QVBoxLayout* mvs_cb_layout = new QVBoxLayout();
    mvs_cb_layout->setSpacing(0);
    mvs_cb_layout->addWidget(&this->mvs_color_scale);
    mvs_cb_layout->addWidget(&this->mvs_write_ply);
    mvs_cb_layout->addWidget(&this->mvs_dz_map);
    mvs_cb_layout->addWidget(&this->mvs_conf_map);
    mvs_cb_layout->addWidget(&this->mvs_auto_save);
    QVBoxLayout* mvs_but_layout = new QVBoxLayout();
    mvs_but_layout->setSpacing(1);
    mvs_but_layout->addWidget(dmrecon_but);
    mvs_but_layout->addWidget(dmrecon_batch_but);
    QFormLayout* mvs_layout = new QFormLayout();
    mvs_layout->addRow(mvs_entry_layout);
    mvs_layout->addRow(mvs_cb_layout);
    mvs_layout->addRow(mvs_but_layout);

    /* Bilateral filter layout. */
    this->bf_gc_sigma.setRange(0.01, 10.0);
    this->bf_gc_sigma.setValue(2.0f);
    this->bf_pc_factor.setRange(0.5, 20.0);
    this->bf_pc_factor.setValue(5.0f);

    QPushButton* filter_but = new QPushButton("Bilateral Filter");
    QFormLayout* bf_layout = new QFormLayout();
    bf_layout->setVerticalSpacing(1);
    bf_layout->addRow(tr("Source"), &this->bf_src_image);
    bf_layout->addRow(tr("Dest"), &this->bf_dst_image);
    bf_layout->addRow(tr("GC sigma"), &this->bf_gc_sigma);
    bf_layout->addRow(tr("PC factor"), &this->bf_pc_factor);
    bf_layout->addRow(filter_but);

    /* Depthmap cleanup layout. */
    this->dmclean_island_size.setRange(1, 10000);
    this->dmclean_island_size.setValue(200);

    QPushButton* dmclean_but = new QPushButton("Clean depthmap");
    QFormLayout* dmclean_layout = new QFormLayout();
    dmclean_layout->setVerticalSpacing(1);
    dmclean_layout->addRow(tr("Source"), &this->dmclean_src_image);
    dmclean_layout->addRow(tr("Dest"), &this->dmclean_dst_image);
    dmclean_layout->addRow(tr("Island size"), &this->dmclean_island_size);
    dmclean_layout->addRow(dmclean_but);

    QCollapsible* mvs_header = new QCollapsible
        ("DM Reconstruct", get_wrapper(mvs_layout));
    QCollapsible* bilateral_header = new QCollapsible
        ("Bilateral filter", get_wrapper(bf_layout));
    QCollapsible* dmclean_header = new QCollapsible
        ("DM cleanup", get_wrapper(dmclean_layout));
    bilateral_header->set_collapsed(true);
    dmclean_header->set_collapsed(true);

    /* Main Layout. */
    QVBoxLayout* main_layout = new QVBoxLayout(this);
    main_layout->setSpacing(5);
    main_layout->addWidget(this->selected_view);
    main_layout->addWidget(mvs_header);
    main_layout->addWidget(bilateral_header);
    main_layout->addWidget(dmclean_header);
    main_layout->addStretch(1);

    /* Connecting signals. */
    this->connect(dmrecon_but, SIGNAL(clicked()), this, SLOT(exec_dmrecon()));
    this->connect(filter_but, SIGNAL(clicked()), this, SLOT(exec_bilateral()));
    this->connect(dmclean_but, SIGNAL(clicked()), this, SLOT(exec_dmclean()));
    this->connect(dmrecon_batch_but, SIGNAL(clicked()),
        this, SLOT(exec_dmrecon_batch()));

    this->connect(&SceneManager::get(), SIGNAL(view_selected(mve::View::Ptr)),
        this, SLOT(on_view_selected(mve::View::Ptr)));
}

/* ---------------------------------------------------------------- */

void
ImageOperationsWidget::on_view_selected (mve::View::Ptr view)
{
    this->selected_view->set_view(view);
    this->selected_view->fill_embeddings(this->mvs_color_image, mve::IMAGE_TYPE_UINT8, "undistorted");
    this->selected_view->fill_embeddings(this->dmclean_src_image, mve::IMAGE_TYPE_FLOAT);
    this->selected_view->fill_embeddings(this->bf_src_image, mve::IMAGE_TYPE_FLOAT);
}

/* ---------------------------------------------------------------- */

void
ImageOperationsWidget::exec_bilateral (void)
{
    mve::View::Ptr view = SceneManager::get().get_view();
    if (view.get() == 0)
    {
        std::cout << "No view set!" << std::endl;
        return;
    }

    std::string src_img = this->bf_src_image.currentText().toStdString();
    std::string dst_img = this->bf_dst_image.text().toStdString();
    float gc_sigma = this->bf_gc_sigma.value();
    float pc_factor = this->bf_pc_factor.value();

    if (src_img.empty() || dst_img.empty())
    {
        std::cout << "Source/dest image name not given" << std::endl;
        return;
    }

    mve::FloatImage::Ptr img = view->get_float_image(src_img);
    if (!img.get())
    {
        std::cout << "Cannot request image: " << src_img << std::endl;
        return;
    }

    mve::CameraInfo const& cam = view->get_camera();
    math::Matrix3f invproj;
    cam.fill_inverse_calibration(*invproj, img->width(), img->height());
    mve::FloatImage::Ptr ret = mve::image::depthmap_bilateral_filter
        (img, invproj, gc_sigma, pc_factor);

    view->set_image(dst_img, ret);
    emit this->signal_select_embedding(QString(dst_img.c_str()));
    emit this->signal_reload_embeddings();
}

/* ---------------------------------------------------------------- */

struct JobDMRecon : public JobProgress
{
    /* Settings. */
    mve::Scene::Ptr scene;
    mve::View::Ptr view;
    mvs::Settings settings;
    bool auto_save;

    /* Status. */
    QFuture<void> future;
    std::string name;
    std::string message;
    mvs::Progress* progress;
    bool thread_started;

    JobDMRecon (void) : progress(0), thread_started(false) {}
    char const* get_name (void) { return name.c_str(); }
    bool is_completed (void) { return future.isFinished(); }
    bool has_progress (void) { return false; }
    float get_progress (void) { return 0.0f; }
    char const* get_message (void)
    {
        if (!this->thread_started)
            return "Waiting for slot";
        if (this->message.empty() && this->progress == 0)
            return "Waiting...";
        if (!this->message.empty() && this->progress == 0)
            return this->message.c_str();
        if (this->progress && this->progress->cancelled)
            return "Cancelling...";
        switch (this->progress->status)
        {
        case mvs::RECON_IDLE: return "MVS is idle";
        case mvs::RECON_GLOBALVS: return "Global VS...";
        case mvs::RECON_FEATURES: return "Processing Features...";
        case mvs::RECON_SAVING: return "Saving reconstruction...";
        case mvs::RECON_CANCELLED: return "Cancelled";
        case mvs::RECON_QUEUE:
        {
            std::stringstream ss;
            ss << "Queue: " << progress->queueSize;
            this->message = ss.str();
            return this->message.c_str();
        }
        default: return "Unknown status";
        }
    }
    void cancel_job (void)
    {
        this->progress->cancelled = true;
        std::cout << "Should cancel job \"" << name << "\"" << std::endl;
        std::cout << "  Running: " << this->future.isRunning() << std::endl
            << "  Paused: " << this->future.isPaused() << std::endl
            << "  Finished: " << this->future.isFinished() << std::endl
            << "  Started: " << this->future.isStarted() << std::endl
            << "  Canceled: " << this->future.isCanceled() << std::endl;
    }
};

void
ImageOperationsWidget::exec_dmrecon (void)
{
    mve::View::Ptr view = SceneManager::get().get_view();
    this->start_dmrecon_job(view);
}

/* ---------------------------------------------------------------- */

void
ImageOperationsWidget::start_dmrecon_job (mve::View::Ptr view)
{
    mve::Scene::Ptr scene = SceneManager::get().get_scene();
    if (scene.get() == 0)
    {
        std::cout << "No scene set!" << std::endl;
        return;
    }
    if (view.get() == 0)
    {
        std::cout << "No view set!" << std::endl;
        return;
    }

    if (!view.get() || !view->is_camera_valid())
    {
        std::cout << "Invalid view selected" << std::endl;
        QMessageBox::warning(this, tr("MVS reconstruct"),
            tr("View invalid or master view has invalid camera!"));
        return;
    }

    /* Create job and read GUI config. */
    JobDMRecon* job = new JobDMRecon();
    job->name = "MVS - " + view->get_name();
    job->scene = scene;
    job->view = view;
    job->auto_save = this->mvs_auto_save.isChecked();

    job->settings.refViewNr = view->get_id();
    job->settings.imageEmbedding = this->mvs_color_image.currentText().toStdString();
    job->settings.globalVSMax = this->mvs_amount_gvs.value();
    job->settings.useColorScale = this->mvs_color_scale.isChecked();
    job->settings.scale = this->mvs_scale.value();
    job->settings.writePlyFile = this->mvs_write_ply.isChecked();
    job->settings.keepConfidenceMap = this->mvs_conf_map.isChecked();
    job->settings.keepDzMap = this->mvs_dz_map.isChecked();
    job->settings.plyPath = scene->get_path();
    job->settings.plyPath += "/recon/";
    job->settings.logPath = scene->get_path();
    job->settings.logPath += "/log/";

    /* Launch and register job. */
    job->future = QtConcurrent::run(this,
        &ImageOperationsWidget::threaded_dmrecon, job);
    JobQueue::get()->add_job(job);
}

/* ---------------------------------------------------------------- */

void
ImageOperationsWidget::threaded_dmrecon (JobDMRecon* job)
{
    job->thread_started = true;

    /* Start MVS reconstruction. */
    try
    {
        mvs::DMRecon recon(job->scene, job->settings);
        job->progress = &recon.getProgress();
        recon.start();

        if (job->progress->cancelled)
            job->message = "Cancelled!";
        else
            job->message = "Finished.";

        job->progress = 0;
        std::cout << "Reconstruction finished!" << std::endl;

    }
    catch (std::exception& e)
    {
        job->progress = 0;
        job->message = "Failed!";
        std::cout << "Reconstruction failed: " << e.what() << std::endl;
        return;
    }

    /* Save view if requested. */
    if (job->auto_save)
    {
        try
        {
            job->view->save_mve_file();
        }
        catch (std::exception& e)
        {
            std::cout << "Saving view failed: " << e.what() << std::endl;
        }
    }

    /* Finalize operation. */
    //emit this->signal_select_embedding("depthmap");
    emit this->signal_reload_embeddings();
}

/* ---------------------------------------------------------------- */

void
ImageOperationsWidget::exec_dmrecon_batch (void)
{
    mve::Scene::Ptr scene = SceneManager::get().get_scene();
    if (scene.get() == 0)
    {
        std::cout << "No scene set!" << std::endl;
        return;
    }

    int scale = this->mvs_scale.value();
    int ret = QMessageBox::question(this, tr("MVS batch reconstruct"),
        tr("Really reconstruct ALL depth maps at scale %1?\n"
        "Note: Existing depth maps are not reconstructed.\n"
        "Note: This feature is still experimental!").arg(scale),
        QMessageBox::Yes, QMessageBox::No);
    if (ret != QMessageBox::Yes)
        return;

    /* Prefetch bundle file. */
    scene->get_bundle();

    std::string dmname("depth-L" + util::string::get(scale));

    bool added = false;
    mve::Scene::ViewList& views(scene->get_views());
    for (std::size_t i = 0; i < views.size(); ++i)
    {
        if (!views[i].get() || !views[i]->is_camera_valid())
            continue;
        if (views[i]->has_embedding(dmname))
            continue;
        this->start_dmrecon_job(views[i]);
        added = true;
    }

    if (!added)
    {
        QMessageBox::information(this, tr("MVS batch reconstruct"),
            tr("The selected scale is already reconstructed in all views."));
    }
}

/* ---------------------------------------------------------------- */

void
ImageOperationsWidget::exec_dmclean (void)
{
    mve::View::Ptr view = SceneManager::get().get_view();
    if (view.get() == 0)
    {
        std::cout << "No view set!" << std::endl;
        return;
    }

    std::string src_img = this->dmclean_src_image.currentText().toStdString();
    std::string dst_img = this->dmclean_dst_image.text().toStdString();
    std::size_t csize = this->dmclean_island_size.value();

    if (src_img.empty() || dst_img.empty())
    {
        std::cout << "Source/dest image name not given" << std::endl;
        return;
    }

    mve::FloatImage::Ptr img = view->get_float_image(src_img);
    if (!img.get())
    {
        std::cout << "Cannot request image: " << src_img << std::endl;
        return;
    }

    mve::FloatImage::Ptr ret = mve::image::depthmap_cleanup(img, csize);

    view->set_image(dst_img, ret);
    emit this->signal_select_embedding(QString(dst_img.c_str()));
    emit this->signal_reload_embeddings();
}
