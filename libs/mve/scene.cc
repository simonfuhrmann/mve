#include <iostream>
#include <algorithm>

#include "util/exception.h"
#include "util/inifile.h"
#include "util/timer.h"
#include "util/filesystem.h"

#include "scene.h"

MVE_NAMESPACE_BEGIN

void
Scene::load_scene (std::string const& base_path)
{
    if (base_path.empty())
        throw util::Exception("Invalid file name given");

    this->basedir = base_path;

    /* Load scene. */
    this->init_views();
}

/* ---------------------------------------------------------------- */

void
Scene::save_scene (void)
{
    this->save_bundle();
    this->save_views();
}

/* ---------------------------------------------------------------- */

void
Scene::save_bundle (void)
{
    if (!this->bundle.get() || !this->bundle_dirty)
        return;
    this->bundle->write_bundle(this->basedir + "/" + "synth_0.out");
    this->bundle_dirty = false;
}

/* ---------------------------------------------------------------- */

void
Scene::save_views (void)
{
    for (std::size_t i = 0; i < this->views.size(); ++i)
        if (this->views[i].get() && this->views[i]->is_dirty())
        {
            View::Ptr const& view = this->views[i];
            std::cout << "Saving view ID " << view->get_id() << std::endl;
            view->save_mve_file();
        }

    std::cout << "Done saving views." << std::endl;
}

/* ---------------------------------------------------------------- */

void
Scene::rewrite_all_views (void)
{
    for (std::size_t i = 0; i < this->views.size(); ++i)
    {
        View::Ptr const& view = this->views[i];
        if (!view.get())
            continue;

        std::cout << "Rewriting view ID " << view->get_id() << std::endl;
        view->save_mve_file(true);
    }

    std::cout << "Done rewriting views." << std::endl;
}

/* ---------------------------------------------------------------- */

bool
Scene::is_dirty (void) const
{
    if (this->bundle_dirty)
        return true;

    for (std::size_t i = 0; i < this->views.size(); ++i)
        if (this->views[i].get() && this->views[i]->is_dirty())
        {
            std::cout << "View " << i << " is dirty." << std::endl;
            return true;
        }

    return false;
}

/* ---------------------------------------------------------------- */

void
Scene::cache_cleanup (void)
{
    if (this->bundle.use_count() == 1)
        this->bundle.reset();

    std::size_t released = 0;
    std::size_t affected_views = 0;
    for (std::size_t i = 0; i < this->views.size(); ++i)
    {
        if (!this->views[i].get())
            continue;

        std::size_t num = this->views[i]->cache_cleanup();
        if (num)
        {
            released += num;
            affected_views += 1;
        }
    }

    std::cout << "Cleanup: Released " << released << " embeddings in "
        << affected_views << " of " << views.size() << " views." << std::endl;
}

/* ---------------------------------------------------------------- */

std::size_t
Scene::get_total_mem_usage (void)
{
    std::size_t ret = 0;
    ret += this->get_view_mem_usage();
    ret += this->get_bundle_mem_usage();
    return ret;
}

/* ---------------------------------------------------------------- */

std::size_t
Scene::get_view_mem_usage (void)
{
    std::size_t ret = 0;
    for (std::size_t i = 0; i < this->views.size(); ++i)
        if (this->views[i].get())
            ret += this->views[i]->get_byte_size();
    return ret;
}

/* ---------------------------------------------------------------- */

std::size_t
Scene::get_bundle_mem_usage (void)
{
    return (this->bundle.get() ? this->bundle->get_byte_size() : 0);
}

/* ---------------------------------------------------------------- */

void
Scene::init_views (void)
{
    util::WallTimer timer;

    /* Iterate over all mve files and create view. */
    std::string views_path = this->basedir + "/" MVE_SCENE_VIEWS_DIR;
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
    std::size_t max_id = 0;
    for (std::size_t i = 0; i < views_dir.size(); ++i)
    {
        if (views_dir[i].name.size() < 4)
            continue;
        if (util::string::right(views_dir[i].name, 4) != ".mve")
            continue;
        View::Ptr view = View::create();
        view->load_mve_file(views_dir[i].get_absolute_name());
        temp_list.push_back(view);
        max_id = std::max(max_id, view->get_id());
    }

    if (max_id > 5000 && max_id > 2 * temp_list.size())
        throw util::Exception("Spurious view IDs");

    /* Transfer temp list to real list. */
    this->views.clear();
    if (!temp_list.empty())
        this->views.resize(max_id + 1);
    for (std::size_t i = 0; i < temp_list.size(); ++i)
    {
        std::size_t id = temp_list[i]->get_id();

        if (this->views[id].get() != 0)
        {
            std::cout << "Warning loading MVE file "
                << this->views[id]->get_filename() << std::endl
                << "  View with ID " << id << " already present"
                << ", skipping file."
                << std::endl;
            continue;
        }

        this->views[id] = temp_list[i];
    }

    std::cout << "Initialized " << temp_list.size()
        << " views (max ID is " << max_id << "), took "
        << timer.get_elapsed() << "ms." << std::endl;
}

/* ---------------------------------------------------------------- */

BundleFile::ConstPtr
Scene::get_bundle (void)
{
    if (!this->bundle.get())
    {
        BundleFile::Ptr b = BundleFile::create();
        b->read_bundle(this->basedir + "/" MVE_SCENE_BUNDLE_FILE);
        this->bundle = b;
        this->bundle_dirty = false;
    }
    return this->bundle;
}

MVE_NAMESPACE_END
