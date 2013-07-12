#ifndef UMVE_SCENEOPERATIONS_HEADER
#define UMVE_SCENEOPERATIONS_HEADER

#include <QSpinBox>
#include <QWidget>

#include "mve/scene.h"

#ifdef USE_BUNDLER
#include "../../../../bundler/lib/bundler.h"
#endif

class SceneOperations : public QWidget
{
    Q_OBJECT

private slots:
    void on_scene_selected (mve::Scene::Ptr scene);
    void on_update_scene (void);

#ifdef USE_BUNDLER
    void on_init_bundler (void);
    void on_full_ba (void);
    void on_run_steps (void);
    void on_error_color (void);
    void on_true_color (void);
    void on_hilight_camera (void);
    void on_undistort (void);
#endif

private:
    void update_scene (void);


private:
    mve::Scene::Ptr scene;

#ifdef USE_BUNDLER
    mve::Bundler bundler;
    QSpinBox hilight_cam;
    QSpinBox run_steps;
#endif

public:
    SceneOperations (void);
};

#endif /* UMVE_SCENEOPERATIONS_HEADER */
