/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef MVE_SCENE_HEADER
#define MVE_SCENE_HEADER

#include <vector>
#include <string>
#include <memory>

#include "mve/defines.h"
#include "mve/view.h"
#include "mve/bundle.h"

#define MVE_SCENE_VIEWS_DIR "views/"
#define MVE_SCENE_BUNDLE_FILE "synth_0.out"

MVE_NAMESPACE_BEGIN

/**
 * Scene representation for the MVE.
 * A scene is loaded by specifying the scene root directory. The following
 * files and directories are recognized within the scene root:
 *
 * - directory "views": contains the views in the scene.
 * - file "synth_0.out": bundle file that contains key points.
 */
class Scene
{
public:
    typedef std::shared_ptr<Scene> Ptr;
    typedef std::vector<View::Ptr> ViewList;

public:
    /** Constructs an unmanaged scene, which should not be copied. */
    Scene (void);

    /** Constructs a smart pointered scene. */
    static Scene::Ptr create (void);
    /** Constructs and loads a scene from the given directory. */
    static Scene::Ptr create (std::string const& path);

    /** Loads the scene from the given directory. */
    void load_scene (std::string const& base_path);

    /** Returns the list of views. */
    ViewList const& get_views (void) const;
    /** Returns the list of views. */
    ViewList& get_views (void);
    /** Returns the bundle structure. */
    Bundle::ConstPtr get_bundle (void);
    /** Sets a new bundle structure. */
    void set_bundle (Bundle::Ptr bundle);
    /** Resets the bundle file such that it is re-read on get_bundle. */
    void reset_bundle (void);

    /** Returns a view by ID or 0 on failure. */
    View::Ptr get_view_by_id (std::size_t id);

    /** Saves bundle file if dirty as well as dirty embeddings. */
    void save_scene (void);
    /** Saves dirty embeddings only. */
    void save_views (void);
    /** Saves the bundle file if dirty. */
    void save_bundle (void);
    /** Forces rewriting of all views. Can take a long time. */
    void rewrite_all_views (void);

    /** Returns true if one of the views or the bundle file is dirty. */
    bool is_dirty (void) const;

    /** Returns the base path of the scene. */
    std::string const& get_path (void) const;

    /** Forces cleanup of unused embeddings. */
    void cache_cleanup (void);

    /** Returns total scene memory usage. */
    std::size_t get_total_mem_usage (void);
    /** Returns view memory usage. */
    std::size_t get_view_mem_usage (void);
    /** Returns key point memory usage. */
    std::size_t get_bundle_mem_usage (void);

private:
    std::string basedir;
    ViewList views;
    Bundle::Ptr bundle;
    bool bundle_dirty;

private:
    void init_views (void);
};

/* ---------------------------------------------------------------- */

inline
Scene::Scene (void)
    : bundle_dirty(false)
{
}

inline Scene::Ptr
Scene::create (void)
{
    return Scene::Ptr(new Scene);
}

inline Scene::Ptr
Scene::create (std::string const& path)
{
    Scene::Ptr scene(new Scene);
    scene->load_scene(path);
    return scene;
}

inline Scene::ViewList const&
Scene::get_views (void) const
{
    return this->views;
}

inline Scene::ViewList&
Scene::get_views (void)
{
    return this->views;
}

inline View::Ptr
Scene::get_view_by_id (std::size_t id)
{
    return (id < this->views.size() ? this->views[id] : View::Ptr());
}

inline std::string const&
Scene::get_path (void) const
{
    return this->basedir;
}

inline void
Scene::set_bundle (Bundle::Ptr bundle)
{
    this->bundle_dirty = true;
    this->bundle = bundle;
}

inline void
Scene::reset_bundle (void)
{
    this->bundle.reset();
    this->bundle_dirty = false;
}

MVE_NAMESPACE_END

#endif /* MVE_SCENE_HEADER */
