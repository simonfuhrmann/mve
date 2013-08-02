#include <set>
#include <sstream>
#include <iostream>

#include <QCoreApplication>
#include <QFileDialog>
#include <QFormLayout>
#include <QMessageBox>
#include <QProgressDialog>

#include "util/filesystem.h"
#include "mve/plyfile.h"
#include "mve/imagefile.h"
#include "mve/imagetools.h"

#include "guihelpers.h"
#include "batchoperations.h"

BatchOperations::BatchOperations (QWidget* parent)
    : QDialog(parent)
{
    /* Setup button bar. */
    QPushButton* close_but = new QPushButton
        (QIcon(":/images/icon_close.svg"), "Close");
    this->button_box.addWidget(close_but, 0);
    this->button_box.addWidget(get_separator(), 1);

    /* Pack everything together. */
    QVBoxLayout* main_layout = new QVBoxLayout();
    main_layout->addLayout(&this->main_box, 1);
    main_layout->addLayout(&this->button_box, 0);
    this->setLayout(main_layout);

    /* Signals... */
    this->connect(close_but, SIGNAL(clicked()),
        this, SLOT(on_close_clicked()));
}

void
BatchOperations::get_embedding_names (mve::ImageType type,
    std::vector<std::string>* result)
{
    if (this->scene == NULL)
        return;

    typedef std::set<std::string> StringSet;
    StringSet names;

    mve::Scene::ViewList const& views(this->scene->get_views());
    for (std::size_t i = 0; i < views.size(); ++i)
    {
        if (views[i] == NULL)
            continue;
        mve::View::Proxies const& p(views[i]->get_proxies());
        for (std::size_t j = 0; j < p.size(); ++j)
            if (type == mve::IMAGE_TYPE_UNKNOWN || type == p[j].get_type())
                names.insert(p[j].name);
    }

    std::copy(names.begin(), names.end(), std::back_inserter(*result));
}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

BatchDelete::BatchDelete (QWidget* parent)
    : BatchOperations(parent)
{
    this->embeddings_list = new QListWidget();
    this->main_box.addWidget(new QLabel("Please select the embeddings "
        "you want to delete in ALL views."));
    this->main_box.addWidget(this->embeddings_list);

    /* Append execute button to button box. */
    QPushButton* batchdel_exec_but = new QPushButton
        (QIcon(":/images/icon_delete.svg"), "Delete!");
    this->button_box.addWidget(batchdel_exec_but, 0);

    /* Signals... */
    this->connect(batchdel_exec_but, SIGNAL(clicked()),
        this, SLOT(on_batchdel_exec()));
}

/* ---------------------------------------------------------------- */

void
BatchDelete::setup_gui (void)
{
    this->embeddings_list->clear();

    if (this->scene == NULL)
        return;

    std::vector<std::string> names;
    this->get_embedding_names(mve::IMAGE_TYPE_UNKNOWN, &names);
    for (std::size_t i = 0; i < names.size(); ++i)
    {
        QListWidgetItem* item = new QListWidgetItem(this->embeddings_list);
        item->setText(names[i].c_str());
        item->setCheckState(Qt::Unchecked);
    }
}

/* ---------------------------------------------------------------- */

void
BatchDelete::on_batchdel_exec (void)
{
    typedef std::set<std::string> StringSet;
    StringSet names;

    int const rows = this->embeddings_list->count();
    for (int i = 0; i < rows; ++i)
    {
        QListWidgetItem const* item = this->embeddings_list->item(i);
        if (item->checkState() == Qt::Checked)
            names.insert(item->text().toStdString());
    }

    std::size_t deleted = 0;
    mve::Scene::ViewList& vl(this->scene->get_views());
    for (std::size_t i = 0; i < vl.size(); ++i)
    {
        mve::View::Ptr view = vl[i];
        if (view == NULL)
            continue;

        for (StringSet::iterator j = names.begin(); j != names.end(); ++j)
        {
            bool removed = view->remove_embedding(*j);
            if (removed)
            {
                deleted += 1;
                std::cout << "Removed \"" << *j
                    << "\" in view " << i << std::endl;
            }
        }
    }

    std::stringstream ss;
    ss << "Deleted a total of " << deleted << " embeddings.\n"
        << "Note that the scene still needs to be saved manually!";
    QMessageBox::information(this, "Deleted embeddings!",
        ss.str().c_str(), QMessageBox::Ok);

    this->setup_gui();
}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

BatchExport::BatchExport (QWidget *parent)
    : BatchOperations(parent)
{
    QPushButton* exec_but = new QPushButton
        (QIcon(":/images/icon_export_ply.svg"), "Export!");
    QPushButton* dirselect_but = new QPushButton
        (QIcon(":/images/icon_open_file.svg"), "");
    dirselect_but->setIconSize(QSize(18, 18));
    dirselect_but->setFlat(true);

    QHBoxLayout* dirselect_box = new QHBoxLayout();
    dirselect_box->addWidget(&this->exportpath, 10);
    dirselect_box->addWidget(dirselect_but, 0);

    QFormLayout* form = new QFormLayout();
    form->setSpacing(2);
    form->addRow(new QLabel("Type the names of the embeddings "
        "you want to export.\nOnly a name for the depthmap is required. "));
    form->addRow("Depthmap:", &this->depthmap_combo);
    form->addRow("Confidence map:", &this->confmap_combo);
    form->addRow("Color Image:", &this->colorimage_combo);
    form->addRow("Path (optional): ", dirselect_box);

    this->main_box.addLayout(form);
    this->button_box.addWidget(exec_but, 0);

    /* Signals... */
    this->connect(exec_but, SIGNAL(clicked()), this, SLOT(on_export_exec()));
    this->connect(dirselect_but, SIGNAL(clicked()), this, SLOT(on_dirselect()));
}

/* ---------------------------------------------------------------- */

void
BatchExport::setup_gui (void)
{
    if (this->scene == NULL)
        return;

    std::vector<std::string> float_names;
    std::vector<std::string> byte_names;
    this->get_embedding_names(mve::IMAGE_TYPE_FLOAT, &float_names);
    this->get_embedding_names(mve::IMAGE_TYPE_UINT8, &byte_names);

    this->depthmap_combo.addItem("");
    this->confmap_combo.addItem("");
    this->colorimage_combo.addItem("");
    for (std::size_t i = 0; i < float_names.size(); ++i)
    {
        this->depthmap_combo.addItem(float_names[i].c_str());
        this->confmap_combo.addItem(float_names[i].c_str());
    }
    for (std::size_t i = 0; i < byte_names.size(); ++i)
        this->colorimage_combo.addItem(byte_names[i].c_str());
}

/* ---------------------------------------------------------------- */

void
BatchExport::on_dirselect (void)
{
    QString dirname = QFileDialog::getExistingDirectory(this,
        "Select export path...");
    if (dirname.isEmpty())
        return;

    this->exportpath.setText(dirname);
}

/* ---------------------------------------------------------------- */

void
export_view_intern (mve::View::Ptr view, std::string const& basename,
    std::string const& depthmap_name, std::string const& confmap_name,
    std::string const& colorimage_name)
{
    if (view->get_float_image(depthmap_name) == NULL)
        return;

    try
    {
        mve::geom::save_ply_view(view, basename + ".ply",
            depthmap_name, confmap_name, colorimage_name);
        mve::geom::save_xf_file(basename + ".xf", view->get_camera());
    }
    catch (std::exception& e)
    {
        std::cout << "Skipping view " << view->get_name()
            << ": " << e.what() << std::endl;
    }
}

void
BatchExport::on_export_exec (void)
{
    std::cout << "Exporting PLY files..." << std::endl;

    /* Setup output path. */
    std::string path;
    if (this->exportpath.text().isEmpty())
        path = this->scene->get_path() + "/recon/";
    else
        path = this->exportpath.text().toStdString();

    if (path.empty())
    {
        QMessageBox::critical(this, "Error exporting!",
            "The destination path is unset.", QMessageBox::Ok);
        return;
    }

    if (*path.rbegin() != '/')
        path.push_back('/');

    /* Create output dir if not exists. */
    if (!util::fs::dir_exists(path.c_str()))
    {
        if (!util::fs::mkdir(path.c_str()))
        {
            QMessageBox::critical(this, "Error exporting!",
                "Error creating output directory.", QMessageBox::Ok);
            return;
        }
    }

    std::string dm_name = this->depthmap_combo.currentText().toStdString();
    std::string cm_name = this->confmap_combo.currentText().toStdString();
    std::string ci_name = this->colorimage_combo.currentText().toStdString();

    /* Prepare views to be exported... */
    mve::Scene::ViewList& views(this->scene->get_views());

    /* Create progress information dialog. */
    QProgressDialog* win = new QProgressDialog("Exporting PLY files...",
        "Cancel", 0, views.size(), this);
    win->setAutoClose(true);
    win->setMinimumDuration(0);

    /* Iterate over views and write to file... */
    for (std::size_t i = 0; i < views.size(); ++i)
    {
        mve::View::Ptr view(views[i]);
        if (view == NULL)
            continue;

        std::stringstream ss;
        ss << "Exporting view ID" << i << " (" << view->get_name() << ")...";
        win->setLabelText(ss.str().c_str());
        win->setValue(i);
        QCoreApplication::processEvents();
        if (win->wasCanceled())
            break;

        std::stringstream basename;
        basename << path << "view_" << view->get_name() << "-" << dm_name;
        export_view_intern(view, basename.str(), dm_name, cm_name, ci_name);
    }
    win->setValue(views.size());

    std::cout << "Done exporting PLY files." << std::endl;
}

/* ---------------------------------------------------------------- */

BatchImportImages::BatchImportImages (QWidget* parent)
    : BatchOperations(parent)
{
    this->create_thumbnails.setText("Create Thumbnails (recommended for UMVE)");
    this->filenames_become_viewnames.setText("Use file names (without extension) as view names");
    this->reuse_view_ids.setText("Reuse unused view IDs (otherwise apped IDs only)");
    this->save_exif_info.setText("Save EXIF information in embedding if available");
    this->create_thumbnails.setChecked(true);
    this->filenames_become_viewnames.setChecked(true);
    this->save_exif_info.setChecked(true);
    this->reuse_view_ids.setChecked(false);
    this->embedding_name.setText("original");

    QPushButton* exec_but = new QPushButton
        (QIcon(":/images/icon_export_ply.svg"), "Import!");

    QFormLayout* form = new QFormLayout();
    form->setSpacing(2);
    form->addRow(&this->selected_files);
    form->addRow("Embedding name:", &this->embedding_name);
    form->addRow(&this->create_thumbnails);
    form->addRow(&this->filenames_become_viewnames);
    form->addRow(&this->save_exif_info);
    form->addRow(&this->reuse_view_ids);

    this->main_box.addLayout(form);
    this->button_box.addWidget(exec_but, 0);
    this->connect(exec_but, SIGNAL(clicked()), this, SLOT(on_import_images()));
}

void
BatchImportImages::setup_gui (void)
{
    this->file_list = QFileDialog::getOpenFileNames(this,
        "Select images for import", QDir::currentPath());
    this->file_list.sort();
    for (int i = 0; i < this->file_list.size(); ++i)
    {
        std::cout << "File name: " << this->file_list[i].toStdString() << std::endl;
    }
    this->selected_files.setText(tr("<b>%1 files have been selected.</b>")
        .arg(this->file_list.size()));
}

void
BatchImportImages::on_import_images (void)
{
    /* The list of views where new views are added to. */
    mve::Scene::ViewList& views = this->scene->get_views();

    bool add_thumbnail = this->create_thumbnails.isChecked();
    bool reuse_ids = this->reuse_view_ids.isChecked();
    bool filenames_as_names = this->filenames_become_viewnames.isChecked();
    bool save_exif = this->save_exif_info.isChecked();

    std::string embedding_name = this->embedding_name.text().toStdString();
    std::string thumbnail_name = "thumbnail";
    std::string exif_name = "exif";

    int const thumb_width = 50;
    int const thumb_height = 50;

    std::size_t last_reused_id = 0;

    std::size_t num_successful = 0;
    std::size_t num_errors = 0;

    /* Create a view for each file, save view and add to scene. */
    for (int i = 0; i < this->file_list.size(); ++i)
    {
        /* Check filename. */
        std::string filename = this->file_list[i].toStdString();
        if (!util::fs::file_exists(filename.c_str()))
        {
            std::cout << "Skipping invalid file: " << filename << std::endl;
            num_errors += 1;
            continue;
        }

        /* Load image. */
        std::cout << "Importing " << filename << "..." << std::endl;
        mve::ByteImage::Ptr image;
        std::string exif_data;
        if (save_exif)
        {
            try { image = mve::image::load_jpg_file(filename, &exif_data); }
            catch (...) {}
        }

        try
        {
            if (image == NULL)
                image = mve::image::load_file(filename);
        }
        catch (std::exception& e)
        {
            std::cout << "Error loading file: " << filename << std::endl;
            std::cout << "  " << e.what() << std::endl;
            num_errors += 1;
            continue;
        }

        /* Find next unused view ID. */
        std::size_t view_id = views.size();
        if (reuse_ids)
        {
            for (; last_reused_id < views.size(); ++last_reused_id)
            {
                if (views[last_reused_id].get() == NULL)
                {
                    view_id = last_reused_id;
                    break;
                }
            }
            last_reused_id += 1;
            if (last_reused_id >= views.size())
                reuse_ids = false;
        }

        /* Find view name (either from ID or file name). */
        std::string view_name = util::string::get_filled(view_id, 4, '0');
        if (filenames_as_names)
        {
            std::size_t pathsep_pos = filename.find_last_of('/');
            std::size_t extension_pos = filename.find_last_of('.');
            if (pathsep_pos == std::string::npos)
                pathsep_pos = 0;
            if (extension_pos == std::string::npos)
                extension_pos = filename.size();
            std::size_t start_pos = pathsep_pos + 1;
            std::size_t num_chars = extension_pos - start_pos;
            view_name = filename.substr(start_pos, num_chars);
        }

        /* Create view and set headers. */
        mve::View::Ptr view = mve::View::create();
        view->set_id(view_id);
        view->set_name(view_name);
        view->add_image(embedding_name, image);

        if (add_thumbnail)
        {
            mve::ByteImage::Ptr thumb = mve::image::create_thumbnail<uint8_t>
                (image, thumb_width, thumb_height);
            view->add_image(thumbnail_name, thumb);
        }

        if (save_exif && !exif_data.empty())
        {
            mve::ByteImage::Ptr exif_image = mve::ByteImage::create(exif_data.size(), 1, 1);
            std::copy(exif_data.begin(), exif_data.end(), exif_image->begin());
            view->add_data(exif_name, exif_image);
        }

        /* Save view to disc. */
        std::string scene_path = this->scene->get_path();
        filename = "view_" + util::string::get_filled(view_id, 4) + ".mve";
        view->save_mve_file_as(scene_path + "/views/" + filename);

        /* Add view to scene. */
        if (views.size() <= view_id)
            views.resize(view_id + 1);
        views[view_id] = view;
        num_successful += 1;
    }

    std::stringstream info_message;
    info_message << "Successfully added: " << num_successful << std::endl;
    info_message << "Images with errors: " << num_errors << std::endl;
    QMessageBox::information(this, "Import complete",
        info_message.str().c_str());

    /* Close the dialog. */
    this->accept();
}
