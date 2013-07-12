#ifndef UMVE_SCENE_INSPECT_HEADER
#define UMVE_SCENE_INSPECT_HEADER

#include "ogl/opengl.h"

#include <vector>

#include <QGLWidget>

#include "mve/scene.h"

#include "glwidget.h"
#include "meshlist.h"
#include "guicontext.h"
#include "sceneoperations.h"

class SceneInspect : public QWidget
{
    Q_OBJECT

private:
    GLWidget* glw;
    QTabWidget* scene_details;

    QAction* action_open_mesh;
    QAction* action_reload_shaders;
    QAction* action_show_details;
    QAction* action_save_screenshot;

    std::vector<GuiContext*> contexts;
    SceneOperations* scene_operations;
    std::size_t current_context;

private:
    void create_actions (void);
    void activate_context (std::size_t i);

private slots:
    void on_open_mesh (void);
    void on_reload_shaders (void);
    void on_details_toggled (void);
    void on_tab_changed (int id);
    void on_save_screenshot (void);
    void on_scene_selected (mve::Scene::Ptr scene);
    void on_view_selected (mve::View::Ptr view);

public:
    SceneInspect (QWidget* parent = 0);

    /* Loads a mesh from file and adds to mesh list. */
    void load_file (std::string const& filename);

    /* Removes references to the scene. */
    void reset (void);
};

#endif /* UMVE_SCENE_INSPECT_HEADER */
