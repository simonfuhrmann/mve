/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <iostream>
#include <algorithm>

#include "util/exception.h"
#include "util/timer.h"
#include "util/file_system.h"
#include "mve/scene.h"
#include "mve/bundle_io.h"

MVE_NAMESPACE_BEGIN

void
Scene::load_scene (std::string const& base_path)
{
    if (base_path.empty())
        throw util::Exception("Invalid file name given");
    this->basedir = base_path;
    this->init_views();
}

/* ---------------------------------------------------------------- */

void
Scene::save_bundle (void)
{
    if (!(this->bundle && this->bundle_dirty))
        return;
    std::string filename = util::fs::join_path(this->basedir, "synth_0.out");
    save_mve_bundle(this->bundle, filename);
    this->bundle_dirty = false;
}

/* ---------------------------------------------------------------- */

void
Scene::save_views (void)
{
    std::cout << "Saving views to MVE files..." << std::flush;

    for (auto& view : views)
        if (view && view->is_dirty())
            view->save_view();
    std::cout << " done." << std::endl;
}

/* ---------------------------------------------------------------- */

void
Scene::cache_cleanup (void)
{
    if (this->bundle.use_count() == 1)
        this->bundle.reset();

    std::size_t released = 0;
    std::size_t affected_views = 0;
    std::size_t total_views = 0;
    for (auto& view : this->views)
    {
        if (view)
        {
            ++total_views;
            if (auto num = view->cache_cleanup())
            {
                released += num;
                ++affected_views;
            }
        }
    }

    std::cout << "Cleanup: Released " << released << " embeddings in "
        << affected_views << " of " << total_views << " views." << std::endl;
}

/* ---------------------------------------------------------------- */

std::size_t
Scene::get_total_mem_usage (void)
{
    return this->get_view_mem_usage() + this->get_bundle_mem_usage();
}

/* ---------------------------------------------------------------- */

std::size_t
Scene::get_view_mem_usage (void)
{
    return std::accumulate(views.begin(), views.end(), std::size_t{0},
        [](std::size_t acc, const View::Ptr& view) {
            return acc + (view ? view->get_byte_size() : 0);
        });
}

/* ---------------------------------------------------------------- */

std::size_t
Scene::get_bundle_mem_usage (void)
{
    return (this->bundle ? this->bundle->get_byte_size() : 0);
}

/* ---------------------------------------------------------------- */

void
Scene::init_views (void)
{
    util::WallTimer timer;

    /* Iterate over all mve files and create view. */
    std::string views_path = util::fs::join_path(this->basedir,
        MVE_SCENE_VIEWS_DIR);
    util::fs::Directory views_dir;
    try
    {
        views_dir.scan(views_path);
    }
    catch (util::Exception& e)
    {
        throw util::Exception(views_path + ": ", e.what());
    }
    std::sort(views_dir.begin(), views_dir.end());

    /* Give some feedback... */
    std::cout << "Initializing scene with " << views_dir.size()
        << " views..." << std::endl;

    /* Load views in a temp list. */
    ViewList temp_list;
    int max_id = 0;
    for (const auto& view_file : views_dir)
    {
        if (view_file.name.size() < 4 || util::string::right(view_file.name, 4) != ".mve")
            continue;
        View::Ptr view = View::create();
        view->load_view(view_file.get_absolute_name());
        temp_list.push_back(view);
        max_id = std::max(max_id, view->get_id());
    }

    if (max_id > 5000 && max_id > 2 * (int)temp_list.size())
        throw util::Exception("Spurious view IDs");

    /* Transfer temp list to real list. */
    this->views.clear();
    if (!temp_list.empty())
        this->views.resize(max_id + 1);
    for (const auto& temp_view : temp_list)
    {
        auto& view =  views[temp_view->get_id()];
        if ( view )
        {
            std::cout << "Warning loading MVE file "
                << view << std::endl
                << "  View with ID " << view->get_id() << " already present"
                << ", skipping file."
                << std::endl;
            continue;
        }
        view = temp_view;
    }

    std::cout << "Initialized " << temp_list.size()
        << " views (max ID is " << max_id << "), took "
        << timer.get_elapsed() << "ms." << std::endl;
}

/* ---------------------------------------------------------------- */

Bundle::ConstPtr
Scene::get_bundle (void)
{
    if (this->bundle == nullptr)
    {
        std::string filename = util::fs::join_path(this->basedir,
            MVE_SCENE_BUNDLE_FILE);
        this->bundle = load_mve_bundle(filename);
        this->bundle_dirty = false;
    }
    return this->bundle;
}

MVE_NAMESPACE_END
