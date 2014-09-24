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
#include "ogl/mesh_renderer.h"
#include "sfm/bundler_common.h"
#include "sfm/bundler_features.h"
#include "sfm/bundler_matching.h"
#include "sfm/bundler_init_pair.h"
#include "sfm/bundler_tracks.h"
#include "sfm/bundler_incremental.h"

#include "glwidget.h"
#include "scene_addins/addin_base.h"
#include "scene_addins/addin_axis_renderer.h"
#include "scene_addins/addin_frusta_sfm_renderer.h"

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
    void initialize_options (void);
    void create_sfm_points_renderer (void);
    void update_frusta_renderer (void);
    void apply_clear_color (void);
    void on_set_clear_color (void);
    void on_matching_compute (void);
    void on_prebundle_load (void);
    void on_prebundle_save (void);
    void on_recon_init_pair (void);
    void on_recon_next_camera (void);
    void on_recon_all_cameras (void);
    void on_apply_to_scene (void);

private:
    QVBoxLayout* create_sfm_layout (void);

private:
    AddinState state;
    std::vector<AddinBase*> addins;

    /* Addins and rendering. */
    AddinAxisRenderer* axis_renderer;
    AddinFrustaSfmRenderer* frusta_renderer;
    ogl::MeshRenderer::Ptr sfm_points_renderer;

    /* SfM Features and Matching UI elements. */
    QSpinBox* features_max_pixels;
    QLineEdit* matching_image_embedding;
    QLineEdit* matching_exif_embedding;
    QLineEdit* matching_prebundle_file;

    /* SfM options. */
    sfm::bundler::Features::Options feature_opts;
    sfm::bundler::Matching::Options matching_opts;
    sfm::bundler::InitialPair::Options init_pair_opts;
    sfm::bundler::Tracks::Options tracks_options;
    sfm::bundler::Incremental::Options incremental_opts;

    /* SfM state data. */
    sfm::bundler::PairwiseMatching pairwise_matching;
    sfm::bundler::ViewportList viewports;
    sfm::bundler::InitialPair::Result init_pair_result;
    sfm::bundler::TrackList tracks;
    sfm::bundler::Incremental incremental_sfm;

    /* UI elements. */
    QTabWidget* tab_widget;
    QColor clear_color;
    QCheckBox* clear_color_cb;
};

#endif // UMVE_SFM_CONTROLS_HEADER
