#include <set>
#include <sstream>
#include <iostream>

#include "util/filesystem.h"
#include "mve/plyfile.h"

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

    if (!this->scene.get())
        return;

    typedef std::set<std::string> StringSet;
    StringSet names;

    mve::Scene::ViewList const& vl(this->scene->get_views());
    for (std::size_t i = 0; i < vl.size(); ++i)
    {
        mve::View::ConstPtr view = vl[i];
        if (!view.get())
            continue;
        mve::View::Proxies const& p(view->get_proxies());
        for (std::size_t j = 0; j < p.size(); ++j)
            names.insert(p[j].name);
    }

    for (StringSet::iterator i = names.begin(); i != names.end(); ++i)
    {
        QListWidgetItem* item = new QListWidgetItem(this->embeddings_list);
        item->setText((*i).c_str());
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
        if (!view.get())
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
    form->addRow("Depthmap:", &this->depthmap);
    form->addRow("Confidence map:", &this->confmap);
    form->addRow("Color Image:", &this->colorimage);
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

    std::string d_depthmap = this->depthmap.text().toStdString();
    std::string d_confidence = this->confmap.text().toStdString();
    std::string d_colorimage = this->colorimage.text().toStdString();

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
        if (!view.get())
            continue;

        std::stringstream ss;
        ss << "Exporting view ID" << i << " (" << view->get_name() << ")...";
        win->setLabelText(ss.str().c_str());
        win->setValue(i);
        QCoreApplication::processEvents();
        if (win->wasCanceled())
            break;

        mve::FloatImage::Ptr dm(view->get_float_image(d_depthmap));
        if (!dm.get())
            continue;

        std::string fname = path + "view_" + view->get_name() + "-" + d_depthmap;

        try
        {
            mve::geom::save_ply_view(view, fname + ".ply",
                d_depthmap, d_confidence, d_colorimage);
            mve::geom::save_xf_file(fname + ".xf", view->get_camera());
        }
        catch (std::exception& e)
        {
            std::cout << "Skipping view " << view->get_name()
                << ": " << e.what() << std::endl;
        }
    }
    win->setValue(views.size());

    std::cout << "Done exporting PLY files." << std::endl;
}

