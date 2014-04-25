#ifndef UMVE_ADDIN_OFFSCREEN_RENDERER_HEADER
#define UMVE_ADDIN_OFFSCREEN_RENDERER_HEADER

#include <QWidget>
#include <QPushButton>
#include <QSpinBox>

#include "mve/image.h"
#include "ogl/camera.h"

#include "guihelpers.h"
#include "scene_addins/addin_base.h"

class AddinOffscreenRenderer : public AddinBase
{
    Q_OBJECT

public:
    AddinOffscreenRenderer (void);
    QWidget* get_sidebar_widget (void);
    void set_scene_camera (ogl::Camera* camera);

private slots:
    void on_snapshot (void);
    void on_render_sequence (void);
    void on_play_sequence (bool save = false);
    void on_display_sequence (void);

private:
    mve::ByteImage::Ptr get_offscreen_image (void);

private:
    ogl::Camera* camera;
    QVBoxLayout* main_box;
    QFileSelector* sequence_file;
    QFileSelector* output_frame_dir;
    QPushButton* play_but;
    QSpinBox* width_spin;
    QSpinBox* height_spin;
    bool working_flag;
};

#endif /* UMVE_ADDIN_OFFSCREEN_RENDERER_HEADER */
