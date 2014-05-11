#ifndef UMVE_SFM_CONTROLS_HEADER
#define UMVE_SFM_CONTROLS_HEADER

#include "ogl/opengl.h"

#include <QSpinBox>
#include <QTabWidget>
#include <QCheckBox>
#include <QLineEdit>
#include <QBoxLayout>

#include "mve/scene.h"
#include "mve/view.h"
#include "ogl/context.h"
#include "ogl/camera_trackball.h"

#include "glwidget.h"
#include "scene_addins/addin_base.h"
#include "scene_addins/addin_axis_renderer.h"
#include "scene_addins/addin_sfm_renderer.h"
#include "scene_addins/addin_frusta_renderer.h"
#include "scene_addins/addin_selection.h"

class SfmControls : public QWidget, public ogl::CameraTrackballContext
{
    Q_OBJECT

public:
    SfmControls (GLWidget* gl_widget, QTabWidget* tab_widget);
    virtual ~SfmControls (void) {}

    void set_scene (mve::Scene::Ptr scene);
    void set_view (mve::View::Ptr view);
    void load_shaders (void);

    virtual bool keyboard_event (ogl::KeyboardEvent const& event);
    virtual bool mouse_event (ogl::MouseEvent const& event);

protected:
    void init_impl (void);
    void resize_impl (int old_width, int old_height);
    void paint_impl (void);

protected slots:
    void apply_clear_color (void);
    void on_set_clear_color (void);
    void on_features_compute (void);
private:
    QVBoxLayout* create_sfm_layout (void);

private:
    AddinState state;
    std::vector<AddinBase*> addins;

    /* Addins. */
    AddinAxisRenderer* axis_renderer;
    AddinSfmRenderer* sfm_renderer;
    AddinFrustaRenderer* frusta_renderer;
    AddinSelection* selection;

    /* SfM Features UI elements. */
    QLineEdit* features_image_embedding;
    QLineEdit* features_exif_embedding;
    QLineEdit* features_feature_embedding;
    QSpinBox* features_max_pixels;

    /* UI elements. */
    QTabWidget* tab_widget;
    QColor clear_color;
    QCheckBox* clear_color_cb;
};

#endif // UMVE_SFM_CONTROLS_HEADER
