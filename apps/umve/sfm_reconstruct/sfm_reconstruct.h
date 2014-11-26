#ifndef UMVE_SFM_RECONSTRUCT_HEADER
#define UMVE_SFM_RECONSTRUCT_HEADER

#include "ogl/opengl.h"

#include <QTabWidget>
#include <QString>

#include "mve/view.h"
#include "mve/scene.h"

#include "mainwindowtab.h"
#include "glwidget.h"
#include "sfm_reconstruct/sfm_controls.h"

class SfmReconstruct : public MainWindowTab
{
    Q_OBJECT

public:
    SfmReconstruct (QWidget* parent = NULL);

    /* Removes references to the scene. */
    void reset (void);

    virtual QString get_title (void);

private slots:
    void on_scene_selected (mve::Scene::Ptr scene);
    void on_view_selected (mve::View::Ptr view);
    void on_tab_activated (void);

private:
    SfmControls* sfm_controls;
    QTabWidget* tab_widget;
    GLWidget* gl_widget;
};

#endif /* UMVE_SFM_RECONSTRUCT_HEADER */
