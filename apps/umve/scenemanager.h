/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef UMVE_SCENEMANAGER_HEADER
#define UMVE_SCENEMANAGER_HEADER

#include <QObject>

#include "mve/scene.h"
#include "mve/view.h"
#include "mve/image_base.h"
#include "mve/bundle.h"

/**
 * The currently active scene as well as the selected view are requried
 * throughout the application. In order to reduce passing the scene
 * through a hierarchy of aggregate objects, the scene, the selected
 * view as well as the selected embedding are managed here. Whenever
 * one of the events happens, the appropriate signal is fired.
 *
 * The selection of the scene is typically handled by the main window.
 * The selection of views is handled by the SceneOverview class.
 * The selection of images is handled by the ViewInspect class.
 */
class SceneManager : public QObject
{
    Q_OBJECT

private:
    mve::Scene::Ptr scene;
    mve::View::Ptr view;
    mve::ImageBase::Ptr image;

signals:
    void scene_selected (mve::Scene::Ptr scene);
    void view_selected (mve::View::Ptr view);
    void image_selected (mve::ImageBase::Ptr image);
    void scene_bundle_changed (void);

public:
    SceneManager (void);
    ~SceneManager (void);
    static SceneManager& get (void);

    void select_scene (mve::Scene::Ptr scene);
    void select_view (mve::View::Ptr view);
    void select_image (mve::ImageBase::Ptr image);
    void select_bundle (mve::Bundle::Ptr bundle);

    mve::Scene::Ptr get_scene (void);
    mve::View::Ptr get_view (void);
    mve::ImageBase::Ptr get_image (void);

    void refresh_scene (void);
    void refresh_view (void);
    void refresh_image (void);
    void refresh_bundle (void);

    void reset_scene (void);
    void reset_view (void);
    void reset_image (void);
};

/* ---------------------------------------------------------------- */

inline void
SceneManager::select_scene (mve::Scene::Ptr scene)
{
    this->scene = scene;
    emit this->scene_selected(scene);
}

inline void
SceneManager::select_view (mve::View::Ptr view)
{
    this->view = view;
    emit this->view_selected(view);
}

inline void
SceneManager::select_image (mve::ImageBase::Ptr image)
{
    this->image = image;
    emit this->image_selected(image);
}

inline void
SceneManager::select_bundle (mve::Bundle::Ptr bundle)
{
    this->scene->set_bundle(bundle);
    emit this->scene_bundle_changed();
}

inline mve::Scene::Ptr
SceneManager::get_scene (void)
{
    return this->scene;
}

inline mve::View::Ptr
SceneManager::get_view (void)
{
    return this->view;
}

inline mve::ImageBase::Ptr
SceneManager::get_image (void)
{
    return this->image;
}

inline void
SceneManager::refresh_scene (void)
{
    this->select_scene(this->scene);
}

inline void
SceneManager::refresh_view (void)
{
    this->select_view(this->view);
}

inline void
SceneManager::refresh_image (void)
{
    this->select_image(this->image);
}

inline void
SceneManager::refresh_bundle (void)
{
    emit this->scene_bundle_changed();
}

inline void
SceneManager::reset_scene (void)
{
    this->select_scene(mve::Scene::Ptr());
}

inline void
SceneManager::reset_view (void)
{
    this->select_view(mve::View::Ptr());
}

inline void
SceneManager::reset_image (void)
{
    this->select_image(mve::ImageBase::Ptr());
}

#endif // UMVE_SCENEMANAGER_HEADER
