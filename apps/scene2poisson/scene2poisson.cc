#include <iostream>
#include <fstream>
#include <vector>

#include "mve/scene.h"
#include "mve/image.h"
#include "mve/trianglemesh.h"
#include "mve/plyfile.h"
#include "mve/vertexinfo.h"
#include "mve/depthmap.h"
#include "util/arguments.h"
#include "util/tokenizer.h"

#include "octree.h"

struct AppSettings
{
    std::string scenedir;
    std::string outmesh;
    std::string aabb;
    std::string depth;
    std::string image;
    std::vector<int> imnums;
};

int
main (int argc, char** argv)
{
    /* Setup argument parser. */
    util::Arguments args;
    args.set_exit_on_error(true);
    args.set_nonopt_minnum(2);
    args.set_nonopt_maxnum(2);
    args.set_helptext_indent(25);
    args.set_usage("Usage: scene2pset [ OPTS ] SCENE_DIR MESH_OUT");
    args.set_description(
        "Generates a pointset from all depth maps of a scene by projecting "
        "reconstructed depth values to the world coordinate system and smartly "
        "combining nearby samples according to their footprint.");
    args.add_option('b', "bounding-box", true, "Six comma separated values used as AABB.");
    args.add_option('d', "depth", true, "Name of the depth map embedding [depth-L1]");
    args.add_option('i', "image", true, "Name of the color image embedding [undist-L1]");
    args.add_option('n', "num", true, "IDs of images to include [all]");
    args.parse(argc, argv);

    /* Init default settings. */
    AppSettings conf;
    conf.scenedir = args.get_nth_nonopt(0);
    conf.outmesh = args.get_nth_nonopt(1);
    conf.depth = "depth-L1";
    conf.image = "undist-L1";
    std::string imstring;
    /* Scan arguments. */
    for (util::ArgResult const* arg = args.next_option();
        arg != NULL; arg = args.next_option())
    {
        switch (arg->opt->sopt)
        {
            case 'b': conf.aabb = arg->arg; break;
            case 'd': conf.depth = arg->arg; break;
            case 'i': conf.image = arg->arg; break;
            case 'n': imstring = arg->arg; break;
            default: throw std::runtime_error("Unknown option");
        }
    }

    if (conf.aabb.empty())
    {
        std::cerr << "Error: Bounding box required!" << std::endl;
        return 1;
    }

    /* Convert AABB string. */
    math::Vec3f aabbmin, aabbmax;
    {
        util::Tokenizer tok;
        tok.split(conf.aabb, ',');
        if (tok.size() != 6)
        {
            std::cout << "Error: Invalid AABB given" << std::endl;
            return 1;
        }
        for (int i = 0; i < 3; ++i)
        {
            aabbmin[i] = util::string::convert<float>(tok[i]);
            aabbmax[i] = util::string::convert<float>(tok[i + 3]);
        }
        std::cout << "Got AABB: (" << aabbmin << ") / ("
            << aabbmax << ")" << std::endl;
    }



    /* Iterate over all views and insert points. */
    Octree oct;
    oct.set_aabb(aabbmin, aabbmax);

    /* Load scene and iterate over views. */
    mve::Scene::Ptr scene(mve::Scene::create());
    scene->load_scene(conf.scenedir);
    mve::Scene::ViewList& views(scene->get_views());
    if(imstring.size() != 0)
    {
        util::Tokenizer tok;
        tok.split(imstring, ',');
        conf.imnums.resize(tok.size());
        for (size_t i = 0; i < tok.size(); ++i)
            conf.imnums[i] = util::string::convert<int>(tok[i]);
    }

    for (std::size_t i = 0; i < views.size(); ++i)
    {
        if(imstring.size() != 0 && std::find(conf.imnums.begin(),
            conf.imnums.end(), i) == conf.imnums.end())
            continue;

        mve::View::Ptr view = views[i];
        if (!view.get())
            continue;

        mve::CameraInfo const& cam = view->get_camera();
        if (cam.flen == 0.0f)
            continue;

        mve::FloatImage::Ptr dm = view->get_float_image(conf.depth);
        if (!dm.get())
            continue;

        mve::ByteImage::Ptr ci;
        if (!conf.image.empty())
            ci = view->get_byte_image(conf.image);

        std::cout << "Processing view \"" << view->get_name()
            << "\"" << (ci.get() ? " (with colors)" : "")
            << "..." << std::endl;

        /* Triangulate depth map. */
        mve::TriangleMesh::Ptr mesh;
        mesh = mve::geom::depthmap_triangulate(dm, ci, cam);
        mesh->ensure_normals(false, true);

        mve::TriangleMesh::VertexList const& mverts(mesh->get_vertices());
        mve::TriangleMesh::NormalList const& mvnorm(mesh->get_vertex_normals());
        mve::TriangleMesh::ColorList const& mvcol(mesh->get_vertex_colors());

        mve::VertexInfoList vinfo(mesh);

        for (std::size_t i = 0; i < mverts.size(); ++i)
        {
            mve::MeshVertexInfo vi(vinfo[i]);
            std::vector<float> edges;
            for (std::size_t j = 0; j < vi.verts.size(); ++j)
                edges.push_back((mverts[i] - mverts[vi.verts[j]]).square_norm());
            float fp = std::sqrt(*std::min_element(edges.begin(), edges.end()));

            Point p;
            p.color = math::Vec3f(*mvcol[i]);
            p.pos = mverts[i];
            p.normal = mvnorm[i];
            p.footprint = fp;
            p.confidence = 1.0f;

            oct.insert(p);
        }
    }

    std::cout << "Generating point set from octree..." << std::endl;
    mve::TriangleMesh::Ptr pset(oct.get_pointset(2.0f));

    std::cout << "Writing Poisson format output file..." << std::endl;
    std::ofstream out(conf.outmesh.c_str());
    for (std::size_t i = 0; i < pset->get_vertices().size(); ++i)
    {
        out << pset->get_vertices()[i] << " "
            << pset->get_vertex_normals()[i] * pset->get_vertex_confidences()[i]
            << std::endl;
    }
    out.close();

    return 0;
}
