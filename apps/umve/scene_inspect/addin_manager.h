/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef UMVE_SCENE_ADDIN_MANAGER_HEADER
#define UMVE_SCENE_ADDIN_MANAGER_HEADER

#include "ogl/opengl.h"

#include <string>
#include <vector>

#include <QTabWidget>
#include <QCheckBox>

#include "mve/scene.h"
#include "mve/view.h"
#include "ogl/context.h"
#include "ogl/camera_trackball.h"

#include "glwidget.h"
#include "selectedview.h"
#include "scene_addins/addin_base.h"
#include "scene_addins/addin_axis_renderer.h"
#include "scene_addins/addin_sfm_renderer.h"
#include "scene_addins/addin_frusta_scene_renderer.h"
#include "scene_addins/addin_mesh_renderer.h"
#include "scene_addins/addin_dm_triangulate.h"
#include "scene_addins/addin_offscreen_renderer.h"
#include "scene_addins/addin_rephotographer.h"
#include "scene_addins/addin_aabb_creator.h"
#include "scene_addins/addin_plane_creator.h"
#include "scene_addins/addin_sphere_creator.h"
#include "scene_addins/addin_selection.h"

/*
 * The addin manager sets up the basic OpenGL context, creates the shaders
 * and refers rendering to a set of addins.
 */
class AddinManager : public QWidget, public ogl::CameraTrackballContext
{
    Q_OBJECT

public:
    AddinManager (GLWidget* gl_widget, QTabWidget* tab_widget);
    virtual ~AddinManager (void) {}
    void set_scene (mve::Scene::Ptr scene);
    void set_view (mve::View::Ptr view);

    void load_file (std::string const& filename);
    void load_shaders (void);
    void reset_scene (void);

    virtual bool keyboard_event (ogl::KeyboardEvent const& event);
    virtual bool mouse_event (ogl::MouseEvent const& event);

protected:
    void init_impl (void);
    void resize_impl (int old_width, int old_height);
    void paint_impl (void);

protected slots:
    void on_set_clear_color (void);
    void apply_clear_color (void);
    void on_mesh_generated (std::string const& name,
        mve::TriangleMesh::Ptr mesh);

private:
    AddinState state;
    std::vector<AddinBase*> addins;

    /* Addins. */
    AddinAxisRenderer* axis_renderer;
    AddinSfmRenderer* sfm_renderer;
    AddinFrustaSceneRenderer* frusta_renderer;
    AddinMeshesRenderer* mesh_renderer;
    AddinDMTriangulate* dm_triangulate;
    AddinOffscreenRenderer* offscreen_renderer;
    AddinRephotographer* rephotographer;
    AddinAABBCreator* aabb_creator;
    AddinPlaneCreator* plane_creator;
    AddinSphereCreator* sphere_creator;
    AddinSelection* selection;

    /* UI elements. */
    QTabWidget* tab_widget;
    SelectedView* selected_view_1;
    SelectedView* selected_view_2;
    QColor clear_color;
    QCheckBox* clear_color_cb;
};

#endif /* UMVE_SCENE_ADDIN_MANAGER_HEADER */
