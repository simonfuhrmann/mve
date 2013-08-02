#ifndef UMVE_BATCHOPERATIONS_HEADER
#define UMVE_BATCHOPERATIONS_HEADER

#include <QCheckBox>
#include <QLineEdit>
#include <QListWidget>

#include "mve/scene.h"
#include "mve/imagebase.h"

#include "guihelpers.h"

/*
 * Dialog that allows batch operations.
 */
class BatchOperations : public QDialog
{
    Q_OBJECT

protected:
    mve::Scene::Ptr scene;

    QVBoxLayout main_box;
    QHBoxLayout button_box;

protected:
    virtual void setup_gui (void) = 0;
    void get_embedding_names (mve::ImageType type,
        std::vector<std::string>* result);

private slots:
    void on_close_clicked (void);

public:
    BatchOperations (QWidget* parent = NULL);
    void set_scene (mve::Scene::Ptr scene);
};

/* ---------------------------------------------------------------- */

/*
 * Batch operation dialog that implements deletion of embeddings.
 */
class BatchDelete : public BatchOperations
{
    Q_OBJECT

private:
    QListWidget* embeddings_list;

private:
    void setup_gui (void);

private slots:
    void on_batchdel_exec (void);

public:
    BatchDelete (QWidget* parent = NULL);
};

/* ---------------------------------------------------------------- */

/*
 * Batch operation dialog that implements export of reconstructions.
 */
class BatchExport : public BatchOperations
{
    Q_OBJECT

private:
    QComboBox depthmap_combo;
    QComboBox confmap_combo;
    QComboBox colorimage_combo;
    QLineEdit exportpath;

private:
    void setup_gui (void);

private slots:
    void on_export_exec (void);
    void on_dirselect (void);

public:
    BatchExport (QWidget* parent = NULL);
};

/* ---------------------------------------------------------------- */

/*
 * Implements import of images.
 */
class BatchImportImages : public BatchOperations
{
    Q_OBJECT

private:
    QCheckBox create_thumbnails;
    QCheckBox filenames_become_viewnames;
    QCheckBox save_exif_info;
    QCheckBox reuse_view_ids;
    QLineEdit embedding_name;
    QLabel selected_files;
    QStringList file_list;

private:
    void setup_gui (void);

private slots:
    void on_import_images (void);

public:
    BatchImportImages (QWidget* parent = NULL);
};

/* ---------------------------------------------------------------- */

inline void
BatchOperations::set_scene (mve::Scene::Ptr scene)
{
    this->scene = scene;
    this->setup_gui();
}

inline void
BatchOperations::on_close_clicked (void)
{
    this->accept();
}

#endif /* UMVE_BATCHOPERATIONS_HEADER */
