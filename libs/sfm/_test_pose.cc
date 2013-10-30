#include <iostream>
#include <map>
#include <vector>

#include "util/aligned_memory.h"
#include "util/timer.h"
#include "mve/image.h"
#include "mve/image_tools.h"
#include "mve/image_io.h"
#include "mve/mesh.h"
#include "mve/mesh_tools.h"

#include "surf.h"
#include "sift.h"
#include "matching.h"
#include "pose.h"
#include "correspondence.h"
#include "triangulate.h"
#include "poseransac.h"
#include "visualizer.h"

#define DIM 128

struct SfmFeatureInfo
{
    float pos[2];
    float color[3];
    int track_id;
};

struct SfmImageInfo
{
    mve::ByteImage::Ptr image;
    util::AlignedMemory<float, 16> descr;
    std::vector<std::pair<float, float> > descr_pos;
    std::map<int, SfmFeatureInfo> features;

    void load_image (std::string const& filename);
    void compute_descriptors (void);
    void load_descriptors (std::string const& filename);
    void assign_descriptors (sfm::Sift::Descriptors const& descriptors);
};

void
SfmImageInfo::load_image (std::string const& filename)
{
    std::cout << "Loading " << filename << "..." << std::endl;
    this->image = mve::image::load_file(filename);
}

void
SfmImageInfo::compute_descriptors (void)
{
    /* Compute SIFT features. */
    sfm::Sift::Options sift_options;
    sfm::Sift sift(sift_options);
    sift.set_image(this->image);
    sift.process();
    this->assign_descriptors(sift.get_descriptors());
}

void
SfmImageInfo::assign_descriptors (sfm::Sift::Descriptors const& descriptors)
{
    /* Copy the descriptors to matching buffer and store image coords. */
    this->descr.allocate(DIM * descriptors.size());
    this->descr_pos.resize(descriptors.size());
    float* out_ptr = this->descr.begin();
    for (std::size_t i = 0; i < descriptors.size(); ++i, out_ptr += DIM)
    {
        sfm::Sift::Descriptor const& d = descriptors[i];
        std::copy(d.data.begin(), d.data.end(), out_ptr);
        this->descr_pos[i] = std::make_pair(d.x, d.y);
    }
}

void
SfmImageInfo::load_descriptors (std::string const& filename)
{
    sfm::Sift::Descriptors descr;
    sfm::Sift::load_lowe_descriptors(filename, &descr);
    std::cout << "Loaded " << descr.size() << " descriptors." << std::endl;
    this->assign_descriptors(descr);
}

int
main (int argc, char** argv)
{
   if (argc < 3)
    {
        std::cerr << "Syntax: " << argv[0] << " image1 image2" << std::endl;
        return 1;
    }

    SfmImageInfo image1, image2;
    try
    {
        image1.load_image(argv[1]);
        image2.load_image(argv[2]);
    }
    catch (std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    image1.compute_descriptors();
    image2.compute_descriptors();
    //image1.load_descriptors("/Users/sfuhrmann/Downloads/fountain10.descr");
    //image2.load_descriptors("/Users/sfuhrmann/Downloads/fountain20.descr");

    /* Feature matching. */
    sfm::MatchingResult matching;
    {
        /* Perform matching. */
        sfm::MatchingOptions matchopts;
        matchopts.descriptor_length = DIM;
        matchopts.lowe_ratio_threshold = 0.80f;
        matchopts.distance_threshold = 0.7f;

        util::WallTimer timer;
        sfm::match_features(matchopts,
            image1.descr.begin(), image1.descr_pos.size(),
            image2.descr.begin(), image2.descr_pos.size(), &matching);
        sfm::remove_inconsistent_matches(&matching);
        std::cout << "Two-view matching took " << timer.get_elapsed() << "ms, "
            << sfm::count_consistent_matches(matching) << " matches."
            << std::endl;
    }

    /* Convert matches to RANSAC data structure. */
    sfm::Correspondences matches;
    for (std::size_t i = 0; i < matching.matches_1_2.size(); ++i)
    {
        if (matching.matches_1_2[i] < 0)
            continue;

        sfm::Correspondence match;
        match.p1[0] = image1.descr_pos[i].first;
        match.p1[1] = image1.descr_pos[i].second;
        match.p2[0] = image2.descr_pos[matching.matches_1_2[i]].first;
        match.p2[1] = image2.descr_pos[matching.matches_1_2[i]].second;
        matches.push_back(match);
    }

    {
        mve::ByteImage::Ptr image, tmp_img1 = image1.image, tmp_img2 = image2.image;
        if (image1.image->channels() == 1)
            tmp_img1 = mve::image::expand_grayscale<unsigned char>(image1.image);
        if (image2.image->channels() == 1)
            tmp_img2 = mve::image::expand_grayscale<unsigned char>(image2.image);
        image = sfm::Visualizer::draw_matches(tmp_img1, tmp_img2, matches);
        mve::image::save_file(image, "/tmp/matches-initial.png");
    }

    /* Pose RANSAC. */
    sfm::PoseRansac2D2D::Result ransac_result;
    {
        sfm::PoseRansac2D2D::Options ransac_options;
        ransac_options.max_iterations = 1000;
        ransac_options.threshold = 2.0;
        ransac_options.already_normalized = false;
        sfm::PoseRansac2D2D ransac(ransac_options);

        util::WallTimer timer;
        ransac.estimate(matches, &ransac_result);
        std::cout << "RANSAC took " << timer.get_elapsed() << "ms, "
            << ransac_result.inliers.size() << " inliers." << std::endl;
    }

    /* Create new set of matches from inliners. */
    {
        sfm::Correspondences tmp_matches(ransac_result.inliers.size());
        for (std::size_t i = 0; i < ransac_result.inliers.size(); ++i)
            tmp_matches[i] = matches[ransac_result.inliers[i]];
        std::swap(matches, tmp_matches);
    }

    {
        mve::ByteImage::Ptr image, tmp_img1 = image1.image, tmp_img2 = image2.image;
        if (image1.image->channels() == 1)
            tmp_img1 = mve::image::expand_grayscale<unsigned char>(image1.image);
        if (image2.image->channels() == 1)
            tmp_img2 = mve::image::expand_grayscale<unsigned char>(image2.image);
        image = sfm::Visualizer::draw_matches(tmp_img1, tmp_img2, matches);
        mve::image::save_file(image, "/tmp/matches-ransac.png");
    }

    {
        std::cout << "Two-View matching statistics: " << std::endl;
        int const num_matches = sfm::count_consistent_matches(matching);
        int const num_inliers = ransac_result.inliers.size();
        int const num_descriptors = std::min(image1.descr_pos.size(), image2.descr_pos.size());
        float matching_ratio = (float)num_matches / (float)num_descriptors;
        float inlier_ratio = (float)num_inliers / (float)num_matches;
        std::cout << "  " << image1.descr_pos.size()
            << " and " << image2.descr_pos.size()
            << " descriptors" << std::endl;
        std::cout << "  " << num_matches << " matches (ratio "
            << matching_ratio << "), " <<  num_inliers << " inliers (ratio "
            << inlier_ratio << ")" << std::endl;
    }

    /* Find normalization for inliers and re-compute fundamental. */
    std::cout << "Re-computing fundamental matrix for inliers..." << std::endl;
    sfm::FundamentalMatrix F;
    {
        sfm::Correspondences tmp_matches = matches;
        math::Matrix3d T1, T2;
        sfm::compute_normalization(tmp_matches, &T1, &T2);
        sfm::apply_normalization(T1, T2, &tmp_matches);
        sfm::fundamental_least_squares(tmp_matches, &F);
        sfm::enforce_fundamental_constraints(&F);
        F = T2.transposed() * F * T1;
    }

    /* Compute pose from fundamental matrix. */
    std::cout << "Computing pose..." << std::endl;
    sfm::CameraPose pose1, pose2;
    {
        // Set K-matrices.
        int const width1 = image1.image->width();
        int const height1 = image1.image->height();
        int const width2 = image2.image->width();
        int const height2 = image2.image->height();
        double flen1 = 31.0 / 35.0 * static_cast<double>(std::max(width1, height1));
        double flen2 = 31.0 / 35.0 * static_cast<double>(std::max(width2, height2));
        pose1.set_k_matrix(flen1, width1 / 2.0, height1 / 2.0);

        pose1.init_canonical_form();
        pose2.set_k_matrix(flen2, width2 / 2.0, height2 / 2.0);
        // Compute essential from fundamental.
        sfm::EssentialMatrix E = pose2.K.transposed() * F * pose1.K;
        // Compute pose from essential.
        std::vector<sfm::CameraPose> poses;
        pose_from_essential(E, &poses);
        // Find the correct pose using point test.
        bool found_pose = false;
        for (std::size_t i = 0; i < poses.size(); ++i)
        {
            poses[i].K = pose2.K;
            if (sfm::is_consistent_pose(matches[1], pose1, poses[i]))
            {
                pose2 = poses[i];
                found_pose = true;
                break;
            }
        }
        if (!found_pose)
        {
            std::cout << "Could not find valid pose." << std::endl;
            return 1;
        }
    }

    /* Triangulate all correspondences. */
    std::cout << "Producing point model..." << std::endl;
    mve::TriangleMesh::Ptr mesh = mve::TriangleMesh::create();
    {
        for (std::size_t i = 0; i < matches.size(); ++i)
        {
            math::Vec3d x = sfm::triangulate_match(matches[i], pose1, pose2);
            mesh->get_vertices().push_back(x);
        }
    }
    mve::geom::save_mesh(mesh, "/tmp/pose.ply");

    /*
     * The strategy to add a third view is the following:
     * - Matching between the 3rd and the 1st and 2nd view
     * - Figure out which matches have a corresponding 3D point
     * - Eliminate spurious tracks
     * - Create list of 2D-3D correspondences and estimate pose
     */
    return 0;
}

