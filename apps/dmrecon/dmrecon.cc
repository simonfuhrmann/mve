#include <iomanip>
#include <iostream>
#include <cstdlib>
#include <csignal>

#include "dmrecon/settings.h"
#include "dmrecon/dmrecon.h"
#include "mve/scene.h"
#include "mve/view.h"
#include "util/timer.h"
#include "util/arguments.h"
#include "util/system.h"
#include "util/tokenizer.h"
#include "util/file_system.h"

#include "fancy_progress_printer.h"

enum ProgressStyle
{
    PROGRESS_SILENT,
    PROGRESS_SIMPLE,
    PROGRESS_FANCY
};

FancyProgressPrinter fancyProgressPrinter;

void
reconstruct (mve::Scene::Ptr scene, mvs::Settings settings)
{
    /*
     * Note: destructor of ProgressHandle sets status to failed
     * if setDone() is not called (this happens when an exception
     * is thrown in mvs::DMRecon)
     */
    ProgressHandle handle(fancyProgressPrinter, settings);
    mvs::DMRecon recon(scene, settings);
    handle.setRecon(recon);
    recon.start();
    handle.setDone();
}

void
aabb_from_string (std::string const& str,
    math::Vec3f* aabb_min, math::Vec3f* aabb_max)
{
    util::Tokenizer tok;
    tok.split(str, ',');
    if (tok.size() != 6)
    {
        std::cerr << "Error: Invalid AABB given" << std::endl;
        std::exit(1);
    }

    for (int i = 0; i < 3; ++i)
    {
        (*aabb_min)[i] = tok.get_as<float>(i);
        (*aabb_max)[i] = tok.get_as<float>(i + 3);
    }
    std::cout << "Using AABB: (" << (*aabb_min) << ") / ("
        << (*aabb_max) << ")" << std::endl;
}

int
main (int argc, char** argv)
{
    /* Catch segfaults to print stack traces. */
    ::signal(SIGSEGV, util::system::signal_segfault_handler);

    /* Parse arguments. */
    util::Arguments args;
    args.set_usage(argv[0], "[ OPTIONS ] SCENEDIR");
    args.set_helptext_indent(23);
    args.set_nonopt_minnum(1);
    args.set_nonopt_maxnum(1);
    args.set_exit_on_error(true);
    args.add_option('n', "neighbors", true,
        "amount of neighbor views (global view selection)");
    args.add_option('m', "master-view", true,
        "reconstructs given master view ID only");
    args.add_option('l', "list-view", true,
        "reconstructs given view IDs (given as string \"0-10\")");
    args.add_option('s', "scale", true,
        "reconstruction on given scale, 0 is original [0]");
    args.add_option('f', "filter-width", true,
        "patch size for NCC based comparison [5]");
    args.add_option('\0', "nocolorscale", false,
        "turn off color scale");
    args.add_option('i', "image", true,
        "specify source image embedding [undistorted]");
    args.add_option('\0', "keep-dz", false,
        "store dz map into view");
    args.add_option('\0', "keep-conf", false,
        "store confidence map into view");
    args.add_option('p', "writeply", false,
        "use this option to write the ply file");
    args.add_option('\0', "plydest", true,
        "path suffix appended to scene dir to write ply files");
    args.add_option('\0', "logdest", true,
        "path suffix appended to scene dir to write log files");
    args.add_option('\0', "bounding-box", true,
        "Six comma separated values used as AABB [disabled]");
    args.add_option('\0', "progress", true,
        "progress output style: 'silent', 'simple' or 'fancy'");
    args.add_option('\0', "force", false, "Reconstruct existing depthmaps");
    args.add_option('\0', "max-pixels", true, "Limit image size [disabled]");
    args.parse(argc, argv);

    std::string basePath = args.get_nth_nonopt(0);
    bool writeply = false;
    std::string plyDest("recon");
    std::string logDest("log");
    int master_id = -1;
    bool force_recon = false;
    ProgressStyle progress_style;
    std::size_t max_pixels = 0;

#ifdef _WIN32
    progress_style = PROGRESS_SIMPLE;
#else
    progress_style = PROGRESS_FANCY;
#endif

    mvs::Settings mySettings;
    std::vector<int> listIDs;

    util::ArgResult const * arg;
    while ((arg = args.next_option()))
    {
        if (arg->opt->lopt == "neighbors")
            mySettings.globalVSMax = arg->get_arg<std::size_t>();
        else if (arg->opt->lopt == "nocolorscale")
            mySettings.useColorScale = false;
        else if (arg->opt->lopt == "master-view")
            master_id = arg->get_arg<int>();
        else if (arg->opt->lopt == "list-view")
            args.get_ids_from_string(arg->arg, &listIDs);
        else if (arg->opt->lopt == "scale")
            mySettings.scale = arg->get_arg<int>();
        else if (arg->opt->lopt == "filter-width")
            mySettings.filterWidth = arg->get_arg<unsigned int>();
        else if (arg->opt->lopt == "image")
            mySettings.imageEmbedding = arg->get_arg<std::string>();
        else if (arg->opt->lopt == "keep-dz")
            mySettings.keepDzMap = true;
        else if (arg->opt->lopt == "keep-conf")
            mySettings.keepConfidenceMap = true;
        else if (arg->opt->lopt == "writeply")
            writeply = true;
        else if (arg->opt->lopt == "plydest")
            plyDest = arg->arg;
        else if (arg->opt->lopt == "logdest")
            logDest = arg->arg;
        else if (arg->opt->lopt == "bounding-box")
            aabb_from_string(arg->arg, &mySettings.aabbMin, &mySettings.aabbMax);
        else if (arg->opt->lopt == "progress")
        {
            if (arg->arg == "silent")
                progress_style = PROGRESS_SILENT;
            else if (arg->arg == "simple")
                progress_style = PROGRESS_SIMPLE;
            else if (arg->arg == "fancy")
                progress_style = PROGRESS_FANCY;
            else
                std::cerr << "WARNING: unrecognized progress style" << std::endl;
        }
        else if (arg->opt->lopt == "force")
            force_recon = true;
        else if (arg->opt->lopt == "max-pixels")
            max_pixels = arg->get_arg<std::size_t>();
        else
        {
            std::cerr << "WARNING: unrecognized option" << std::endl;
        }
    }

    /* don't show progress twice */
    if (progress_style != PROGRESS_SIMPLE)
        mySettings.quiet = true;

    /* Load MVE scene. */
    mve::Scene::Ptr scene(mve::Scene::create());
    try
    {
        scene->load_scene(basePath);
        scene->get_bundle();
    }
    catch (std::exception& e)
    {
        std::cerr << "Error loading scene: " << e.what() << std::endl;
        return 1;
    }

    /* Settings for Multi-view stereo */
    mySettings.writePlyFile = writeply; // every time this is set to true, a kitten is killed
    mySettings.plyPath = util::fs::join_path(basePath, plyDest);
    mySettings.logPath = util::fs::join_path(basePath, logDest);

    fancyProgressPrinter.setBasePath(basePath);
    fancyProgressPrinter.setNumViews(scene->get_views().size());

    if (progress_style == PROGRESS_FANCY)
        fancyProgressPrinter.pt_create();

    util::WallTimer timer;
    if (master_id >= 0)
    {
        std::cout << "Reconstructing view with ID " << master_id << std::endl;
        mySettings.refViewNr = (std::size_t)master_id;
        /* Calculate scale */
        if (max_pixels > 0)
        {
            int w = scene->get_view_by_id(master_id)->get_proxy(mySettings.imageEmbedding)->width;
            int h = scene->get_view_by_id(master_id)->get_proxy(mySettings.imageEmbedding)->height;
            mySettings.scale = std::max(0,
                (int)std::ceil(std::log(1.0f * w * h / max_pixels) / std::log(4)));
        }
        fancyProgressPrinter.addRefView(master_id);
        try
        {
            reconstruct(scene, mySettings);
        }
        catch (std::exception &err)
        {
            std::cerr << err.what() << std::endl;
        }
    }
    else
    {
        mve::Scene::ViewList& views(scene->get_views());
        if (listIDs.empty())
        {
            std::cout << "Reconstructing all views..." << std::endl;
            for(std::size_t i = 0; i < views.size(); ++i)
                listIDs.push_back(i);
        }
        else
        {
            std::cout << "Reconstructing views from list..." << std::endl;
        }
        fancyProgressPrinter.addRefViews(listIDs);

#pragma omp parallel for schedule(dynamic, 1)
        for (std::size_t i = 0; i < listIDs.size(); ++i)
        {
            std::size_t id = listIDs[i];
            if (id >= views.size())
            {
                std::cout << "Invalid ID " << id << ", skipping!" << std::endl;
                continue;
            }
            if (views[id] == NULL || !views[id]->is_camera_valid())
                continue;

            /* Calculate scale */
            int scale = mySettings.scale;
            if (max_pixels > 0)
            {
                int w = views[id]->get_proxy(mySettings.imageEmbedding)->width;
                int h = views[id]->get_proxy(mySettings.imageEmbedding)->height;
                scale = std::max(0,
                    (int)std::ceil(std::log(1.0f * w * h / max_pixels) / std::log(4)));
            }
            std::string embedding_name = "depth-L" +
                util::string::get(scale);
            if (!force_recon && views[id]->has_embedding(embedding_name))
                continue;

            mvs::Settings settings(mySettings);
            settings.scale = scale;
            settings.refViewNr = id;
            try
            {
                reconstruct(scene, settings);
                views[id]->save_mve_file();
            }
            catch (std::exception &err)
            {
                std::cerr << err.what() << std::endl;
            }
        }
    }

    if (progress_style == PROGRESS_FANCY)
    {
        fancyProgressPrinter.stop();
        fancyProgressPrinter.pt_join();
    }

    std::cout << "Reconstruction took "
        << timer.get_elapsed() << "ms." << std::endl;

    /* Save scene */
    std::cout << "Saving views back to disc..." << std::endl;
    scene->save_views();

    return 0;
}
