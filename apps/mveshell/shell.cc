#include <iostream>
#include <set>

#define USE_GNU_READLINE
#ifdef USE_GNU_READLINE
#   include <stdlib.h>
#   include <readline/readline.h>
#   include <readline/history.h>
#endif

#include "util/tokenizer.h"
#include "util/filesystem.h"
#include "mve/scene.h"
#include "mve/imagefile.h"
#include "mve/imagetools.h"
#include "mve/imageexif.h"

#include "shell.h"

Shell::Shell (void)
{
    std::cout << "Welcome to the MVE shell. Try \"help\"." << std::endl;

    while (true)
    {
        std::string line = readline();
        if (line == "exit" || line == "quit")
            break;

        try
        {
            this->process_line(line);
        }
        catch (std::exception& e)
        {
            std::cout << "Error: " << e.what() << std::endl;
        }
    }
}

/* ---------------------------------------------------------------- */

std::string
Shell::readline (void)
{
#ifdef USE_GNU_READLINE

        char* cline = ::readline("mve> ");

        /* EOF? */
        if (!cline)
        {
            std::cout << std::endl;
            return "exit";
        }

        /* Non-empty line? */
        if (*cline)
            ::add_history(cline);

        std::string line(cline);
        ::free(cline);
        return line;

#else

    std::cout << "mve> " << std::flush;
    std::string line;
    std::getline(std::cin, line);
    return line;

#endif
}

/* ---------------------------------------------------------------- */

void
Shell::process_line (std::string const& line)
{
    util::Tokenizer tok;
    tok.split(line);

    if (tok.empty())
        return;

    if (tok.size() == 1 && tok[0] == "help")
    {
        this->print_help();
    }
    else if (tok.size() == 2 && tok[0] == "open")
    {
        this->scene = mve::Scene::create();
        this->scene->load_scene(tok[1]);
    }
    else if (tok.size() == 2 && tok[0] == "delete")
    {
        this->delete_embeddings(tok[1]);
    }
    else if (tok.size() == 2 && tok[0] == "list")
    {
        if (tok[1] == "embeddings")
            this->list_embeddings();
    }
    else if (tok.size() >= 2 && tok[0] == "export")
    {
        std::string path = (tok.size() == 3 ? tok[2] : "");
        this->export_embeddings(tok[1], path);
    }
    else if (tok.size() == 1 && tok[0] == "save")
    {
        this->scene->save_views();
    }
    else if (tok.size() == 2 && tok[0] == "addexif")
    {
        this->add_exif(tok[1]);
    }
    else
    {
        std::cout << "Unknown command: " << line << std::endl;
        std::cout << "Try \"help\" for available commands." << std::endl;
    }
}

/* ---------------------------------------------------------------- */

void
Shell::print_help (void)
{
    std::cout << "List of available commands" << std::endl
        << "  open DIR           Open MVE scene" << std::endl
        << "  delete NAME        Delete embeddings NAME from all views" << std::endl
        << "  list embeddings    Print list of all embeddings" << std::endl
        << "  export NAME [PATH] Export embeddings NAME from all views" << std::endl
        << "  addexif IMG_PATH   Adds EXIF tags to views form source images" << std::endl
        << "  save               Write changes to MVE scene" << std::endl
        << "  exit               Exit MVE shell" << std::endl
        << "  help               Print this help" << std::endl;
}

/* ---------------------------------------------------------------- */

void
Shell::delete_embeddings (std::string const& name)
{
    if (!this->scene.get())
        throw std::runtime_error("No scene loaded");

    std::size_t num_removed = 0;
    mve::Scene::ViewList& vl(this->scene->get_views());
    for (std::size_t i = 0; i < vl.size(); ++i)
    {
        mve::View::Ptr view = vl[i];
        if (!view.get())
            continue;

        bool removed = view->remove_embedding(name);
        if (removed)
            num_removed += 1;
    }

    std::cout << "Deleted " << num_removed << " embeddings." << std::endl;
}

/* ---------------------------------------------------------------- */

void
Shell::list_embeddings (void)
{
    if (!this->scene.get())
        throw std::runtime_error("No scene loaded");

    typedef std::set<std::string> StringSet;
    StringSet names;

    mve::Scene::ViewList const& vl(this->scene->get_views());
    for (std::size_t i = 0; i < vl.size(); ++i)
    {
        mve::View::ConstPtr view = vl[i];
        if (!view.get())
            continue;
        mve::View::Proxies const& p(view->get_proxies());
        for (std::size_t j = 0; j < p.size(); ++j)
            names.insert(p[j].name);
    }

    std::cout << "List of embedding names in all views:" << std::endl;
    for (StringSet::iterator i = names.begin(); i != names.end(); ++i)
    {
        std::cout << "    " << *i << std::endl;
    }
}

/* ---------------------------------------------------------------- */

void
export_byte_image (mve::ByteImage::Ptr image, std::string const& filename)
{
#if 0 // Hack to export resized square images
    #define THUMB_SIZE 500
    #define THUMB_SIZEF 500.0f
    std::size_t iw = image->width();
    std::size_t ih = image->height();

    if (iw < THUMB_SIZE || ih < THUMB_SIZE)
    {
        std::cout << "too small, skipping." << std::endl;
        return;
    }

    std::size_t dw, dh, dl, dt;
    if (iw > ih)
    {
        dh = THUMB_SIZE;
        dw = (std::size_t)(THUMB_SIZEF * (float)iw / (float)ih);
        dl = (dw - THUMB_SIZE) / 2;
        dt = 0;
    }
    else
    {
        dw = THUMB_SIZE;
        dh = (std::size_t)(THUMB_SIZEF * (float)ih / (float)iw);
        dl = 0;
        dt = (dh - THUMB_SIZE) / 2;
    }

    mve::ByteImage::Ptr ret = mve::image::rescale<uint8_t>
        (image, mve::image::RESCALE_LINEAR, dw, dh);
    ret = mve::image::crop<uint8_t>(ret, dl, dt, THUMB_SIZE, THUMB_SIZE);
    image = ret;
#endif

    try
    {
        mve::image::save_file(image, filename);
        std::cout << "exported." << std::endl;
    }
    catch (std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
}

void
export_float_image (mve::FloatImage::Ptr image, std::string const& filename)
{
    float min = std::numeric_limits<float>::max();
    float max = -std::numeric_limits<float>::min();
    std::size_t value_amount = image->get_value_amount();
    for (std::size_t i = 0; i < value_amount; ++i)
    {
        float value(image->at(i));
        if (value == 0.0f)
            continue;
        min = std::min(value, min);
        max = std::max(value, max);
    }

    mve::ByteImage::Ptr img = mve::image::float_to_byte_image(image, min, max);
    export_byte_image(img, filename);
}

void
Shell::export_embeddings (std::string const& name, std::string const& path)
{
    if (!this->scene.get())
        throw std::runtime_error("No scene loaded");

    /* Create output directory. */
    std::string destdir = path;
    if (path.empty())
    {
        destdir = this->scene->get_path() + "/images/";
        if (!util::fs::dir_exists(destdir.c_str()))
            if (!util::fs::mkdir(destdir.c_str()))
            {
                std::cerr << "Error creating output directory" << std::endl;
                return;
            }
    }

    /* Export images... */
    mve::Scene::ViewList& vl(this->scene->get_views());
    for (std::size_t i = 0; i < vl.size(); ++i)
    {
        mve::View::Ptr view = vl[i];
        if (!view.get())
            continue;

        std::cout << "View " << i << " (" << view->get_name() << "): ";
        std::string filename = destdir + name + "_" + view->get_name() + ".png";

        /* Get byte image. */
        mve::ImageBase::Ptr image = view->get_byte_image(name);
        if (image.get())
        {
            export_byte_image(image, filename);
            continue;
        }

        /* No byte image? Check float image! */
        image = view->get_float_image(name);
        if (image.get())
        {
            export_float_image(image, filename);
            continue;
        }

        std::cout << "no such image." << std::endl;
    }
}

/* ---------------------------------------------------------------- */

void
Shell::add_exif (std::string const& path)
{
    if (!this->scene.get())
        throw std::runtime_error("No scene loaded");

    if (!util::fs::dir_exists(path.c_str()))
        throw std::runtime_error("Image path is invalid");

    mve::Scene::ViewList const& vl(this->scene->get_views());
    for (std::size_t i = 0; i < vl.size(); ++i)
    {
        mve::View::Ptr view = vl[i];
        if (!view.get())
            continue;

        std::string name = path + "/" + view->get_name();
        std::string fname = name;
        if (!util::fs::file_exists(fname.c_str()))
            fname = name + ".JPG";
        if (!util::fs::file_exists(fname.c_str()))
            fname = name + ".jpg";
        if (!util::fs::file_exists(fname.c_str()))
        {
            std::cout << "Warning: Cannot find image for view \""
                << name << "\", skipping." << std::endl;
            continue;
        }

        std::cout << "Loading EXIF for " << fname << "..." << std::endl;
        std::string exif;
        mve::image::load_jpg_file(fname, &exif);
        if (exif.empty())
        {
            std::cout << "    does not contain EXIF information." << std::endl;
            continue;
        }

        mve::ByteImage::Ptr data = mve::ByteImage::create(exif.size(), 1, 1);
        std::copy(exif.begin(), exif.end(), data->begin());
        view->set_data("exif", data);
        view->save_mve_file();
    }
}
