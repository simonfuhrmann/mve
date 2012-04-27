#ifndef UMVE_BATCHOPERATIONS_HEADER
#define UMVE_BATCHOPERATIONS_HEADER

#include <QtGui>

#include "mve/scene.h"

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

private slots:
    void on_close_clicked (void);

public:
    BatchOperations (QWidget* parent = 0);
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
    BatchDelete (QWidget* parent = 0);
};

/* ---------------------------------------------------------------- */

/*
 * Batch operation dialog that implements export of reconstructions.
 */
class BatchExport : public BatchOperations
{
    Q_OBJECT

private:
    QLineEdit depthmap;
    QLineEdit confmap;
    QLineEdit colorimage;
    QLineEdit exportpath;

private:
    void setup_gui (void);

private slots:
    void on_export_exec (void);
    void on_dirselect (void);

public:
    BatchExport (QWidget* parent = 0);
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
