#include <iostream>

#include "arguments.h"

int main (int argc, char** argv)
{
    util::Arguments args;
    args.add_option('f', "force", false, "Force operation");
    args.add_option('s', "silent", false, "Slient mode");
    args.add_option('\0', "help", false, "Prints a help text");
    args.add_option('w', "write", true, "Saves to file");
    args.add_option('l', "", true, "Load file");
    args.set_usage("Usage: test [ OPTIONS ] [ non-arg ]");
    args.set_nonopt_maxnum(1);
    args.set_nonopt_minnum(1);
    args.set_exit_on_error(true);
    args.parse(argc, argv);

    util::ArgResult const* res = 0;
    std::size_t i = 0;
    while (res = args.next_result())
    {
        if (res->opt == 0)
        {
            std::cout << "Non-Option " << i << ": " << res->arg << std::endl;
        }
        else
        {
            std::cout << "Option " << i << ": " << res->opt->sopt;
            if (!res->arg.empty())
                std::cout << " (arg: " << res->arg << ")" << std::endl;
            else
                std::cout << std::endl;

            if (res->opt->lopt == "help")
                args.generate_helptext(std::cout);
        }

        i += 1;
    }

    return 0;
}
