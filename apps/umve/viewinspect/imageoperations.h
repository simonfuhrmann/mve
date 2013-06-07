#ifndef IMAGE_OPERATIONS_HEADER
#define IMAGE_OPERATIONS_HEADER

#include <QtGui>

#include "dmrecon/Progress.h"
#include "mve/scene.h"
#include "mve/view.h"

#include "selectedview.h"

class JobDMRecon;

class ImageOperationsWidget : public QWidget
{
    Q_OBJECT

private:
    SelectedView* selected_view;

    /* Bilateral filter settings. */
    QDoubleSpinBox bf_gc_sigma;
    QDoubleSpinBox bf_pc_factor;
    QComboBox bf_src_image;
    QLineEdit bf_dst_image;

    /* Depthmap cleanup settings. */
    QComboBox dmclean_src_image;
    QLineEdit dmclean_dst_image;
    QSpinBox dmclean_island_size;

    /* MVS settings. */
    QSpinBox mvs_amount_gvs;
    QSpinBox mvs_scale;
    QComboBox mvs_color_image;
    QCheckBox mvs_color_scale;
    QCheckBox mvs_write_ply;
    QCheckBox mvs_dz_map;
    QCheckBox mvs_conf_map;
    QCheckBox mvs_auto_save;

private:
    void start_dmrecon_job (mve::View::Ptr view);
    void threaded_dmrecon (JobDMRecon* job);

signals:
    void signal_reload_embeddings (void);
    void signal_select_embedding (QString const& name);

private slots:
    void exec_bilateral (void);
    void exec_dmrecon (void);
    void exec_dmclean (void);
    void exec_dmrecon_batch (void);
    void on_view_selected (mve::View::Ptr view);

public:
    ImageOperationsWidget (void);
};

#endif /* IMAGE_OPERATIONS_HEADER */
