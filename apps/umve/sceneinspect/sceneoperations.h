#ifndef UMVE_SCENEOPERATIONS_HEADER
#define UMVE_SCENEOPERATIONS_HEADER

#include <QSpinBox>
#include <QWidget>

#include "mve/scene.h"

class SceneOperations : public QWidget
{
    Q_OBJECT

public:
    SceneOperations (void);

private slots:
    void on_scene_selected (mve::Scene::Ptr scene);
    void on_update_scene (void);

private:
    void update_scene (void);


private:
    mve::Scene::Ptr scene;
};

#endif /* UMVE_SCENEOPERATIONS_HEADER */
