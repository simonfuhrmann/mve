#include <iomanip>
#include <iostream>
#include <fstream>
#include <cstdlib>

#include "dmrecon/single_view.h"
#include "math/vector.h"
#include "mve/camera.h"
#include "mve/depthmap.h"
#include "mve/plyfile.h"
#include "mve/scene.h"
#include "mve/trianglemesh.h"
#include "mve/bilateral.h"
#include "util/filesystem.h"

int
main (int argc, char** argv)
{
    /* Check arguments. */
    if (argc != 3) {
        std::cerr<<"Syntax: "<<argv[0]
                 <<" <scene path> <master view nr>"<<std::endl;
        return 1;
    }
    unsigned int masterImgNr = atoi(argv[2]);

    /* Load MVE scene. */
    mve::Scene::Ptr scene(mve::Scene::create());
    try {
        scene->load_scene(argv[1]);
    }
    catch (std::exception& e) {
        std::cout<<"Error loading scene: "<<e.what()<<std::endl;
        return 1;
    }

    mve::Scene::ViewList const & views(scene->get_views());
    mve::View::Ptr mview = views[masterImgNr];
    mvs::SingleViewPtr sview(new mvs::SingleView(mview));

    /* Check if master image exists */
    if (masterImgNr > views.size() ||
        (mview.get() == NULL) ||
        (!mview->is_camera_valid()))
    {
        std::cerr<<"ERROR: Master view is invalid."<<std::endl;
        return 1;
    }

    std::vector<mve::FloatImage::Ptr> depthmap;
    std::vector<mve::FloatImage::Ptr> bilateral;
    std::vector<mve::FloatImage::Ptr> normalmap;
    std::vector<mve::FloatImage::Ptr> confmap;
    unsigned int scale = 0;
    
    /* load depthmap, normal map and confidence map for all scales */
    float focal_len = mview->get_camera().flen;
    while (scale < sview->getNrMMlevels()) {
        std::cout << "Load data on scale " << scale << "." << std::endl;
        std::string name("depth-L");
        name += util::string::get(scale);
        if (mview->has_embedding(name)) {
            depthmap.push_back(mview->get_image(name));
            /* apply bilateral filter to depth map */
            bilateral.push_back(mve::image::depthmap_bilateral_filter(
                    depthmap.back(), focal_len ,2.f, 5.f));
            name = "normal-L";
            name += util::string::get(scale);
            normalmap.push_back(mview->get_image(name));
            name = "conf-L";
            name += util::string::get(scale);
            confmap.push_back(mview->get_image(name));
        }
        else
            break;
        ++scale;
    }
    mve::ByteImage::Ptr undistorted(mview->get_image("undistorted"));
    
    if (scale == 0) {
        std::cerr<<"Depth image of scale 0 missing."<<std::endl;
        exit(1);
    }

    unsigned int width = depthmap[0]->width();
    unsigned int height = depthmap[0]->height();
    mve::FloatImage::Ptr depthAll = mve::Image<float>::create();
    depthAll->allocate(width, height, 1);
    mve::FloatImage::Ptr confAll = mve::Image<float>::create();
    confAll->allocate(width, height, 1);

    /* create combined depth image */
    std::cout<<"Create combined image."<<std::endl;
    for (unsigned int j = 0; j < height; ++j) {
        for (unsigned int i = 0; i < width; ++i) {
            unsigned int ii = i;
            unsigned int jj = j;
            confAll->at(i,j,0) = 0.f;
            for (unsigned int s = 0; s < scale; ++s) {
                if (confmap[s]->at(ii,jj,0) > 0) {
                    float depth = depthmap[s]->at(ii,jj,0);
                    if (s == 0) {
                        depthAll->at(i,j,0) = depth;
                        confAll->at(i,j,0) = 1.f / scale;                        
                        break;
                    }
                    float x = ((float) i + 0.5f) / width *
                        depthmap[s]->width() - 0.5f;
                    float y = ((float) j + 0.5f) / height *
                        depthmap[s]->height() - 0.5f;
                    // normalmap contains normal in world coordinates
                    math::Vec3f normal(normalmap[s]->at(ii,jj,0),
                        normalmap[s]->at(ii,jj,1), normalmap[s]->at(ii,jj,2));
                    math::Vec3f v1 = sview->viewRay(ii,jj,s);
                    math::Vec3f v2 = sview->viewRay( x, y,s);
                    if (v1.dot(normal) > 0)
                        std::cerr << "Dot product positive: " << v1.dot(normal)
                                  << std::endl;
                    // case differentiation necessary
                    float phi1, phi2;
                    if ((v2 - v1).dot(normal) > 0) {
                        phi1 = acos(v1.dot(normal)) - mvs::pi / 2.f;
                        phi2 = 1.5f * mvs::pi - acos(v2.dot(normal));
                    }
                    else {
                        phi1 = 1.5f * mvs::pi - acos(v1.dot(normal));
                        phi2 = acos(v2.dot(normal)) - mvs::pi / 2.f;
                    }
                    // check for consistency:
                    //float alpha = acos(std::max(std::min(v1.dot(v2), 1.f), -1.f));
                    //std::cout << alpha << " + " << phi1 << " + " << phi2 << " = "
                    //          << (alpha + phi1 + phi2) << std::endl;
                    depthAll->at(i,j,0) = depth * sin(phi1) / sin(phi2);
                    confAll->at(i,j,0) = (s + 1.f) / scale;
                    break;
                }
                ii /= 2;
                jj /= 2;
            }
        }
    }

    /* save combined depth image to view */
    //mview->set_image("depth-All", depthAll);
    // std::string viewName(argv[1]);
    // viewName += "/recon/mvs-";
    // viewName += util::string::get_filled(masterImgNr, 4);
    // viewName += "-all.ply";
    // mve::geom::save_ply_view(viewName, mview->get_camera(),
    //     depthAll, confAll, undistorted);

    /* triangulate mesh */
    mve::CameraInfo cam(mview->get_camera());
    float dd_factor = 25.0f;
    mve::TriangleMesh::Ptr mesh(mve::geom::depthmap_triangulate(
            depthAll, undistorted, cam.flen, dd_factor));
    std::string meshName(argv[1]);
    meshName += "/recon/mvs-";
    meshName += util::string::get_filled(masterImgNr, 4);
    meshName += "-all.ply";
    mve::geom::save_ply_mesh(mesh, meshName);

    /* Save scene */
    //std::cout<<"Saving views back to disc ..."<<std::endl;
    //scene->save_views();

    return 0;
}
