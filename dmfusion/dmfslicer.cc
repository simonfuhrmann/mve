#include <iostream>
#include <fstream>
#include <limits>
#include <cstring>
#include <cerrno>

#include "util/filesystem.h"
#include "util/arguments.h"
#include "util/timer.h"

#include "mve/image.h"
#include "mve/imagefile.h"

#include "libdmfusion/octree.h"

struct AppSettings
{
    std::string octreefile;
    std::string outdir;
    int level;
    int axis;
    std::size_t start_id;
    std::size_t end_id;
    bool all_slices;
};

/* ---------------------------------------------------------------- */

int
main (int argc, char** argv)
{
    /* Setup argument parser. */
    util::Arguments args;
    args.add_option('l', "level", true, "Octree level to slice");
    args.add_option('a', "axis", true, "Axis orthogonal to slices [y]");
    args.add_option('i', "id", true, "Index of slice along axis");
    args.add_option('s', "start-id", true, "Start index of slice");
    args.add_option('e', "end-id", true, "End index of slice");
    args.add_option('x', "all", false, "Outputs ALL slices along axis");
    args.set_exit_on_error(true);
    args.set_nonopt_maxnum(2);
    args.set_nonopt_minnum(2);
    args.set_helptext_indent(20);
    args.set_usage("Usage: dmfslicer [ OPTIONS ] IN_OCTREE OUT_DIR");
    args.parse(argc, argv);

    /* Setup defaults. */
    AppSettings conf;
    conf.level = -1;
    conf.axis = 1;
    conf.start_id = 0;
    conf.end_id = 0;
    conf.all_slices = false;

    /* Process arguments. */
    int nonopt_iter = 0;
    for (util::ArgResult const* i = args.next_result();
        i != 0; i = args.next_result())
    {
        if (i->opt == 0)
        {
            if (nonopt_iter == 0)
                conf.octreefile = i->arg;
            else if (nonopt_iter == 1)
                conf.outdir = i->arg;
            nonopt_iter += 1;
            continue;
        }

        switch (i->opt->sopt)
        {
            case 'l': conf.level = i->get_arg<int>(); break;
            case 's': conf.start_id = i->get_arg<std::size_t>(); break;
            case 'e': conf.end_id = i->get_arg<std::size_t>(); break;
            case 'x': conf.all_slices = true; break;
            case 'i':
                conf.start_id = i->get_arg<std::size_t>();
                conf.end_id = conf.start_id + 1;
                break;
            case 'a':
                switch (i->arg[0])
                {
                    case 'x': conf.axis = 0; break;
                    case 'y': conf.axis = 1; break;
                    case 'z': conf.axis = 2; break;
                    default:
                        std::cerr << "Invalid axis: " << i->arg[0] << std::endl;
                        return 1;
                }
                break;
            default: break;
        }
    }

    if (conf.all_slices)
    {
        std::size_t one(1);
        conf.start_id = 0;
        conf.end_id = (one << conf.level) + one;
    }

    if (conf.octreefile.empty() || conf.outdir.empty()
        || conf.start_id == conf.end_id || conf.level < 0)
    {
        args.generate_helptext(std::cerr);
        return 1;
    }

    if (!util::fs::dir_exists(conf.outdir.c_str()))
    {
        std::cout << "Creating directory: " << conf.outdir << std::endl;
        bool created = util::fs::mkdir(conf.outdir.c_str());
        if (!created)
        {
            std::cerr << conf.outdir << ": " << ::strerror(errno) << std::endl;
            return 1;
        }
    }

    /* Load octree into memory. */
    util::ClockTimer timer;
    dmf::Octree octree;
    octree.load_octree(conf.octreefile);
    std::cout << "Loading octree took "
        << timer.get_elapsed() << "ms." << std::endl;

    /* Generate slice. */
    timer.reset();
    std::size_t one(1);
    std::size_t dim = (one << conf.level) + one;
    std::size_t maxid = std::min(dim, conf.end_id);
    for (std::size_t id = conf.start_id; id < maxid; ++id)
    {
        mve::FloatImage::Ptr fi = octree.get_slice(conf.level, conf.axis, id);
        std::size_t const w = fi->width();
        std::size_t const h = fi->height();
        float min_dist = std::numeric_limits<float>::max();
        float max_dist = -min_dist;
        float max_weight = 0.0f;
        for (std::size_t i = 0; i < w * h; ++i)
        {
            min_dist = std::min(min_dist, fi->at(i, 0));
            max_dist = std::max(max_dist, fi->at(i, 0));
            max_weight = std::max(max_weight, fi->at(i, 1));
        }

        mve::ByteImage::Ptr bi = mve::ByteImage::create(w, h, 3);
        for (std::size_t i = 0; i < w * h; ++i)
        {
            float weight = fi->at(i, 1);
            if (weight > 0.0f)
            {
                float dist = fi->at(i, 0);
                float red = dist >= 0.0f ? ((max_dist - dist) / max_dist) : 0.0f;
                float green = dist < 0.0f ? ((min_dist - dist) / min_dist) : 0.0f;
                bi->at(i, 0) = (uint8_t)(math::algo::clamp(red, 0.0f, 1.0f) * 255.0f);
                bi->at(i, 1) = (uint8_t)(math::algo::clamp(green, 0.0f, 1.0f) * 255.0f);
                bi->at(i, 2) = 0;
            }
            else
            {
                bi->at(i, 0) = 0;
                bi->at(i, 1) = 0;
                bi->at(i, 2) = 128;
            }
        }
        std::stringstream ss;
        ss << conf.outdir << "/slice-A" << conf.axis
            << "-L" << conf.level << "-ID"
            << util::string::get_filled(id, 4, '0') << ".png";
        std::cout << "Saving file " << ss.str() << "..." << std::endl;
        mve::image::save_file(bi, ss.str());
    }

    std::cout << "Slicing took " << timer.get_elapsed() << "ms." << std::endl;

    return 0;
}
