#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cerrno>

#include "math/quaternion.h"
#include "math/vector.h"
#include "dmfusion/octree.h"
#include "util/tokenizer.h"
#include "util/clocktimer.h"
#include "util/fs.h"

#include "marchingcubes.h"
#include "plyfile.h"
#include "trianglemesh.h"
#include "meshtools.h"
#include "depthmap.h"
#include "image.h"
#include "imagetools.h"
#include "imagefile.h"

//#define DATASET "dragon"
//#define DATASET "drill"
#define DATASET "bunny"
#define OCT_LEVEL 9
#define CONFRANGE 6

struct RangeImage
{
    std::string filename;
    math::Vec3f translation;
    math::Quat4f rotation;
    math::Vec3f campos;
    mve::TriangleMesh::Ptr mesh;
};

typedef std::vector<RangeImage> RangeImages;

void
read_rangeimages (RangeImages& rangeimages, std::string const& conffile)
{
    math::Vec3f camera;
    math::Quat4f camquat;

    std::string basepath = util::fs::get_path_component(conffile);
    basepath += "/";

    std::ifstream in(conffile.c_str());
    if (!in.good())
    {
        std::cout << "Error opening config: " << ::strerror(errno) << std::endl;
        std::cout << "Path: " << conffile << std::endl;
        return;
    }

    while (!in.eof() && in.good())
    {
        std::string buf;
        std::getline(in, buf);
        util::Tokenizer t;
        t.split(buf);

        if (t.empty())
            continue;

        if (t[0] == "camera" && t.size() == 8)
        {
            camera[0] = util::string::convert<float>(t[1]);
            camera[1] = util::string::convert<float>(t[2]);
            camera[2] = util::string::convert<float>(t[3]);
            camquat[0] = util::string::convert<float>(t[4]);
            camquat[1] = util::string::convert<float>(t[5]);
            camquat[2] = util::string::convert<float>(t[6]);
            camquat[3] = util::string::convert<float>(t[7]);

            std::cout << "Camera: " << camera << std::endl;
            std::cout << "Camera quat: " << camquat
                << " (len " << camquat.norm() << ")" << std::endl;
        }
        else if (t[0] == "bmesh" && t.size() == 9)
        {
            RangeImage ri;
            ri.filename = t[1];
            ri.translation = math::Vec3f(util::string::convert<float>(t[2]),
                util::string::convert<float>(t[3]),
                util::string::convert<float>(t[4]));
            ri.rotation[0] = /*-*/util::string::convert<float>(t[8]);
            ri.rotation[1] = util::string::convert<float>(t[5]);
            ri.rotation[2] = util::string::convert<float>(t[6]);
            ri.rotation[3] = util::string::convert<float>(t[7]);

            // http://en.wikipedia.org/wiki/Rotation_matrix
            // http://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation#Pseudo-code_for_rotating_using_a_quaternion_in_3D_space
            // http://de.wikipedia.org/wiki/Quaternion#Konjugation_und_Betrag

            math::Vec3f test_axis;
            float test_angle;
            ri.rotation.get_axis_angle(*test_axis, test_angle);

            std::cout << std::endl;
            std::cout << "Mesh: " << ri.filename << std::endl;
            std::cout << "Rotation: " << test_axis << " (len " << test_axis.norm() << "), angle: " << test_angle << std::endl;
            std::cout << "Translation: " << ri.translation << std::endl;

            ri.mesh = mve::geom::load_mesh(basepath + ri.filename);
            mve::TriangleMesh::VertexList& verts(ri.mesh->get_vertices());
            for (std::size_t i = 0; i < verts.size(); ++i)
                verts[i] = ri.rotation.rotate(verts[i]) + ri.translation;

            ri.campos = ri.rotation.rotate(camquat.rotate(camera));
            ri.mesh->ensure_normals();
            rangeimages.push_back(ri);

#if 0 // Test depthmap_mesh_confidences()
            mve::geom::depthmap_mesh_confidences(ri.mesh, 6);
            mve::TriangleMesh::ConfidenceList& confs(ri.mesh->get_vertex_confidences());
            colors.clear();
            for (std::size_t i = 0; i < confs.size(); ++i)
            {
                colors.push_back(math::Vec4f(1.0f - confs[i], 0.0f, 0.0f, 1.0f));
            }
            mve::geom::save_mesh(ri.mesh, "/tmp/confidence-6.ply");
            return 0;
#endif
        }
        else
        {
            std::cout << "Line not recognized: " << buf << std::endl;
        }

    }
    in.close();
}

/* ---------------------------------------------------------------- */

int main (int argc, char** argv)
{
    /* Read bunny range images and store in world coordinates. */
    //std::string basepath("/gris/gris-f/home/sfuhrman/offmodels/rangeimages/" DATASET "/"/*"/data/"*/);
    //std::string basepath("/data/offmodels/" DATASET "/data/");

    std::vector<std::string> configs;
    configs.push_back("/gris/gris-f/home/sfuhrman/offmodels/rangeimages/bunny/data/bun.conf");
#if 0
    configs.push_back("/gris/gris-f/home/sfuhrman/offmodels/rangeimages/dragon_stand/dragonStandRight.conf");
    configs.push_back("/gris/gris-f/home/sfuhrman/offmodels/rangeimages/dragon_side/dragonSideRight.conf");
    configs.push_back("/gris/gris-f/home/sfuhrman/offmodels/rangeimages/dragon_up/dragonUpRight.conf");
    configs.push_back("/gris/gris-f/home/sfuhrman/offmodels/rangeimages/dragon_fillers/fillers.conf");
    //configs.push_back("/gris/gris-f/home/sfuhrman/offmodels/rangeimages/dragon_backdrop/carvers.conf");
#endif

    /* Read all range images. */
    RangeImages rangeimages;
    for (std::size_t i = 0; i < configs.size(); ++i)
        read_rangeimages(rangeimages, configs[i]);

    if (rangeimages.empty())
    {
        std::cout << "No rangeimages loaded!" << std::endl;
        return 1;
    }

    std::cout << std::endl;
    std::cout << "Range images total: " << rangeimages.size() << std::endl;


    /* Construct octree and fuse meshes. */
    dmf::Octree octree;
    octree.set_allow_expansion(false);
    octree.set_ramp_factor(8.0f);
    octree.set_sampling_rate(1.0f);
    octree.force_octree_level(OCT_LEVEL);

    std::cout << std::endl;
    std::cout << "Inserting into octree at level " << OCT_LEVEL << "..." << std::endl;

    util::ClockTimer timer;
    #if 1
    for (std::size_t i = 0; i < rangeimages.size(); ++i)
    {
        util::ClockTimer mtimer;
        std::cout << "Inserting mesh " << i << "..." << std::endl;
        mve::geom::depthmap_mesh_confidences(rangeimages[i].mesh, CONFRANGE);
        octree.insert(rangeimages[i].mesh, rangeimages[i].campos);
        std::cout << "Inserting mesh took " << mtimer.get_elapsed() << "ms." << std::endl;
    }
    #else
    mve::geom::depthmap_mesh_confidences(rangeimages[0].mesh, CONFRANGE);
    mve::geom::depthmap_mesh_confidences(rangeimages[1].mesh, CONFRANGE);
    octree.insert(rangeimages[0].mesh, rangeimages[0].campos);
    octree.insert(rangeimages[1].mesh, rangeimages[1].campos);
    #endif
    std::cout << "Done inserting into octree, took "
        << timer.get_elapsed() << "ms." << std::endl;

    /* Provide accessor for MC and slicing. */
    dmf::OctreeAccessor accessor(octree.get_root(), OCT_LEVEL);

    std::cout << "Slicing volume..." << std::endl;
    #define SLICES 64
    float dist_thres = octree.get_root()->hs * MATH_SQRT2 * 2.0f / 100.0f;
    std::cout << "Distance threshold " << dist_thres << std::endl;
    for (std::size_t i = 0; i < SLICES; ++i)
    {
        mve::ByteImage::Ptr slice = mve::geom::mc_volume_slicer(accessor, dist_thres, i * accessor.dim[1] / SLICES);
        std::string filename = "/tmp/" DATASET "_slice-" + util::string::get_filled(i, 2) + ".png";
        std::cout << "Saving image " << filename << "..." << std::endl;
        mve::image::save_file(slice, filename);
    }

    timer.reset();
    std::cout << "Starting marching cubes..." << std::endl;
    accessor.min_weight = 0.0f;
    mve::TriangleMesh::Ptr surface = mve::geom::marching_cubes(accessor);
    mve::geom::save_mesh(surface, "/tmp/" DATASET "_merged-L10-RF8-CONF6-CROP00.off");
    std::cout << "Marching cubes took " << timer.get_elapsed() << "ms" << std::endl;

    timer.reset();
    accessor.min_weight = 0.05f;
    surface = mve::geom::marching_cubes(accessor);
    mve::geom::save_mesh(surface, "/tmp/" DATASET "_merged-L10-RF8-CONF6-CROP05.off");
    std::cout << "Marching cubes took " << timer.get_elapsed() << "ms" << std::endl;

    timer.reset();
    accessor.min_weight = 0.1f;
    surface = mve::geom::marching_cubes(accessor);
    mve::geom::save_mesh(surface, "/tmp/" DATASET "_merged-L10-RF8-CONF6-CROP10.off");
    std::cout << "Marching cubes took " << timer.get_elapsed() << "ms" << std::endl;

    return 0;
}
