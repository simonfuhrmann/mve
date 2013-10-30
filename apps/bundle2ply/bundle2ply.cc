#include <iostream>
#include "mve/bundle_file.h"

int
main (int argc, char** argv)
{
    if (argc != 3)
    {
        std::cout << "Syntax: " << argv[0] << " <bundle> <ply>" << std::endl;
        std::exit(1);
    }

    mve::BundleFile bundle;
    try
    { bundle.read_bundle(argv[1]); }
    catch (std::exception& e)
    {
        std::cout << "Error reading bundle: " << e.what() << std::endl;
        std::exit(1);
    }

    try
    { bundle.write_points_to_ply(argv[2]); }
    catch (std::exception& e)
    {
        std::cout << "Error writing PLY: " << e.what() << std::endl;
        std::exit(1);
    }

    return 0;
}
