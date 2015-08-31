#include "math/matrix.h"
#include "math/vector.h"
#include "sfm/ba_interface.h"

namespace
{
    void
    make_camera_pair (sfm::ba::Camera* cam1, sfm::ba::Camera* cam2)
    {
        {
            sfm::ba::Camera* cam = cam1;
            cam->focal_length = 1.0;
            std::fill(cam->distortion + 0, cam->distortion + 2, 0.0);

            double const alpha = MATH_DEG2RAD(45.0);
            cam->rotation[0] = std::cos(alpha);
            cam->rotation[1] = 0;
            cam->rotation[2] = std::sin(alpha);
            cam->rotation[3] = 0;
            cam->rotation[4] = 1;
            cam->rotation[5] = 0;
            cam->rotation[6] = -std::sin(alpha);
            cam->rotation[7] = 0;
            cam->rotation[8] = std::cos(alpha);

            math::Matrix3d rot(cam->rotation);
            math::Vec3d trans = rot * math::Vec3d(-1, 0, 0);
            std::copy(trans.begin(), trans.end(), cam->translation);
        }

        {
            sfm::ba::Camera* cam = cam2;
            cam->focal_length = 1.0;
            std::fill(cam->distortion + 0, cam->distortion + 2, 0.0);

            double const alpha = MATH_DEG2RAD(-45.0);
            cam->rotation[0] = std::cos(alpha);
            cam->rotation[1] = 0;
            cam->rotation[2] = std::sin(alpha);
            cam->rotation[3] = 0;
            cam->rotation[4] = 1;
            cam->rotation[5] = 0;
            cam->rotation[6] = -std::sin(alpha);
            cam->rotation[7] = 0;
            cam->rotation[8] = std::cos(alpha);

            math::Matrix3d rot(cam->rotation);
            math::Vec3d trans = rot * math::Vec3d(1, 0, 0);
            std::copy(trans.begin(), trans.end(), cam->translation);
        }
    }

    void
    make_points (std::vector<sfm::ba::Point3D>* points)
    {
        sfm::ba::Point3D px;
        double* p = px.pos;
        p[0] = +0.0; p[1] = 0.2; p[2] = 1.0;  points->push_back(px);
        p[0] = -0.5; p[1] = 0.4; p[2] = 1.4;  points->push_back(px);
        //p[0] = +0.5; p[1] = 0.0; p[2] = 1.2;  points->push_back(px);
        //p[0] = -0.3; p[1] = 0.1; p[2] = 1.1;  points->push_back(px);
        //p[0] = +0.4; p[1] = -0.2; p[2] = 0.7;  points->push_back(px);
        //p[0] = -0.1; p[1] = -0.4; p[2] = 0.8;  points->push_back(px);
        //p[0] = +0.2; p[1] = -0.3; p[2] = 0.9;  points->push_back(px);
    }

    void
    project (sfm::ba::Camera const& cam, sfm::ba::Point3D const& p3d, double* p2d)
    {
        math::Matrix3d rot(cam.rotation);
        math::Vec3d trans(cam.translation);
        math::Vec3d p(p3d.pos);
        math::Vec3d x = rot * p + trans;
        x /= x[2];
        x *= cam.focal_length;
        p2d[0] = x[0];
        p2d[1] = x[1];
        std::cout << "Projection: " << x << std::endl;
    }
}

int
main (void)
{
    std::vector<sfm::ba::Camera> cams(2);
    make_camera_pair(&cams[0], &cams[1]);

    std::vector<sfm::ba::Point3D> p3d;
    make_points(&p3d);

    std::vector<sfm::ba::Point2D> p2d;
    for (std::size_t i = 0; i < p3d.size(); ++i)
    {
        sfm::ba::Point2D obs1;
        obs1.camera_id = 0;
        obs1.point3d_id = i;
        project(cams[0], p3d[i], obs1.pos);

        sfm::ba::Point2D obs2 = obs1;
        obs2.camera_id = 1;
        project(cams[1], p3d[i], obs2.pos);

        p2d.push_back(obs1);
        p2d.push_back(obs2);
    }

    sfm::ba::BundleAdjustment::Options ba_opts;
    ba_opts.verbose_output = true;
    sfm::ba::BundleAdjustment ba(ba_opts);
    ba.set_cameras(&cams);
    ba.set_points_3d(&p3d);
    ba.set_points_2d(&p2d);
    ba.optimize();
    ba.print_options();
    ba.print_status();

    return 0;
}
