/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <vector>
#include <string>
#include <iostream>
#include <cstdlib>

#include "ogl/opengl.h"

#include <QApplication>
#include <QGLFormat>

#include "util/file_system.h"
#include "util/arguments.h"

#include "mainwindow.h"
#include "guihelpers.h"

struct AppSettings
{
    bool gl_mode;
    bool open_dialog;
    std::vector<std::string> filenames;
};

void
print_help_and_exit (util::Arguments const& args)
{
    args.generate_helptext(std::cerr);
    std::exit(EXIT_FAILURE);
}

int
main (int argc, char** argv)
{
    /* Parse arguments. */
    util::Arguments args;
    args.set_usage("Syntax: umve [ OPTIONS ] [ FILES | SCENEDIR ]");
    args.set_helptext_indent(14);
    args.set_exit_on_error(true);
    args.add_option('h', "help", false, "Prints this help text and exits");
    args.add_option('o', "open-dialog", "Raises scene open dialog on startup");
    args.add_option('\0', "gl", false, "Switches to GL window on startup");
    args.parse(argc, argv);

    /* Set default startup config. */
    AppSettings conf;
    conf.gl_mode = false;
    conf.open_dialog = false;

    /* Handle arguments. */
    util::ArgResult const* arg;
    while ((arg = args.next_result()))
    {
        if (arg->opt == nullptr)
        {
            conf.filenames.push_back(arg->arg);
            continue;
        }

        if (arg->opt->lopt == "gl")
            conf.gl_mode = true;
        else if (arg->opt->lopt == "open-dialog")
            conf.open_dialog = true;
        else
            print_help_and_exit(args);
    }

    /* Set OpenGL version that Qt should use when creating a context.*/
    QGLFormat fmt;
    fmt.setVersion(3, 3);
#if defined(_WIN32)
    fmt.setProfile(QGLFormat::CompatibilityProfile);
#else
    fmt.setProfile(QGLFormat::CoreProfile);
#endif
    QGLFormat::setDefaultFormat(fmt);

    /* Create application. */
    set_qt_style("Cleanlooks");
#if defined(_WIN32)
    QCoreApplication::addLibraryPath(QString::fromStdString(
        util::fs::join_path(util::fs::dirname(util::fs::get_binary_path()),
        "qt_plugins")));
#endif
    QApplication app(argc, argv);

    /* Create main window. */
    MainWindow win;

    /* Apply app config. */
    if (conf.gl_mode)
        win.open_scene_inspect();

    bool scene_opened = false;
    for (std::size_t i = 0; i < conf.filenames.size(); ++i)
    {
        std::string const& filename = conf.filenames[i];
        if (util::fs::file_exists(filename.c_str()))
        {
            win.load_file(filename);
        }
        else if (!scene_opened)
        {
            win.load_scene(filename);
            scene_opened = true;
        }
        else
        {
            std::cerr << "Ignoring extra directory argument: "
                << filename << std::endl;
            continue;
        }
    }

    if (!scene_opened && conf.open_dialog)
        win.raise_open_scene_dialog();

    return app.exec();
}
