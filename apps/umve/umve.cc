#include <iostream>
#include <cstdlib>

#include "ogl/opengl.h"

#include <QApplication>

#include "util/file_system.h"
#include "util/arguments.h"

#include "mainwindow.h"
#include "guihelpers.h"

struct AppSettings
{
    bool gl_mode;
    bool no_open;
    std::string filename;
};

void
print_help_and_exit (util::Arguments const& args)
{
    args.generate_helptext(std::cerr);
    std::exit(1);
}

int
main (int argc, char** argv)
{
    /* Parse arguments. */
    util::Arguments args;
    args.set_usage("Syntax: umve [ OPTIONS ] [ FILE | SCENEDIR ]");
    args.set_helptext_indent(14);
    args.set_nonopt_maxnum(1);
    args.set_exit_on_error(true);
    args.add_option('h', "help", false, "Prints this help text and exits");
    args.add_option('\0', "gl", false, "Switches to GL window on startup");
    args.add_option('\0', "no-open", false, "Does not raise an open dialog");
    args.parse(argc, argv);

    /* Set default startup config. */
    AppSettings conf;
    conf.gl_mode = false;
    conf.no_open = false;

    /* Handle arguments. */
    util::ArgResult const* arg;
    while ((arg = args.next_result()))
    {
        if (arg->opt == NULL)
        {
            conf.filename = arg->arg;
            continue;
        }

        switch (arg->opt->sopt)
        {
            default:
            case 'h': print_help_and_exit(args);
            case '\0': break;
        }

        if (!arg->opt->sopt)
        {
            std::string const& lopt = arg->opt->lopt;
            if (lopt == "gl")
                conf.gl_mode = true;
            else if (lopt == "no-open")
                conf.no_open = true;
            else
                print_help_and_exit(args);
        }
    }

    /* Create application. */
    set_qt_style("Cleanlooks");
    QApplication app(argc, argv);

    /* Create main window. */
    MainWindow win;

    /* Apply app config. */
    if (conf.gl_mode)
        win.open_scene_inspect();

    bool sth_opened = false;
    if (!conf.filename.empty())
    {
        if (util::fs::file_exists(conf.filename.c_str()))
            win.load_file(conf.filename);
        else
            win.load_scene(conf.filename);
        sth_opened = true;
    }

    if (!sth_opened && !conf.no_open)
        win.raise_open_scene_dialog();

    return app.exec();
}
