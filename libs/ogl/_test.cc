#include "camera.h"

int main (void)
{
    ogl::Camera cam;
    cam.viewing_dir = math::Vec3f(-1.0f, 0.0f, -1.0f).normalized();
    cam.pos = math::Vec3f(1.0f, 0.0f, 0.0f);
    cam.up_vec = math::Vec3f(0.0f, 1.0f, 0.0f);

    cam.update_view_mat();

    math::Vec3f p = cam.view.mult(math::Vec3f(0.0f, 0.0f, -1.0f), 1.0f);
    std::cout << p << std::endl;

    return 0;
}
