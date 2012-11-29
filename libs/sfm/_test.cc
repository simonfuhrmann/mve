#include <iostream>

#include "util/timer.h"
#include "mve/image.h"
#include "mve/imagefile.h"

#include "surf.h"

int main (void)
{
    mve::ByteImage::Ptr image;
    try
    {
        image = mve::image::load_file("/tmp/image.jpg");
    }
    catch (std::exception& e)
    {
        std::cout << "Exception: " << e.what() << std::endl;
    }

    sfm::Surf surf;
    surf.set_image(image);
    surf.set_contrast_threshold(25.0f);
    surf.process();

    /* Save result to image. */
    sfm::Surf::SurfKeypoints const& keypoints = surf.get_keypoints();
    for (std::size_t i = 0; i < keypoints.size(); ++i)
    {
        sfm::SurfKeypoint const& kp = keypoints[i];
        image->at((int)kp.x, (int)kp.y, 0) = 255;
        image->at((int)kp.x, (int)kp.y, 1) = (int)(kp.sample * 50.0);
        image->at((int)kp.x, (int)kp.y, 2) = 0;
    }

    mve::image::save_file(image, "/tmp/out.png");

    return 0;
}
