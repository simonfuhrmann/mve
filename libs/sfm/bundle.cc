#include <iostream>
#include <algorithm>

#include "util/timer.h"
#include "mve/trianglemesh.h"
#include "mve/meshtools.h"
#include "sfm/bundle.h"
#include "sfm/triangulate.h"

SFM_NAMESPACE_BEGIN

void
Bundle::add_image (mve::ByteImage::ConstPtr image, double focal_length)
{
    this->viewports.push_back(Viewport());
    this->viewports.back().image = image;
    this->viewports.back().focal_length = focal_length;

    // TODO: Downscaling of the input image
}

void
Bundle::create_bundle (void)
{
    ImagePair initial_pair = this->select_initial_pair();
    this->compute_sift_descriptors(initial_pair.first);
    this->compute_sift_descriptors(initial_pair.second);
    this->two_view_pose(&initial_pair);
    this->triangulate_pair(initial_pair);
}

Bundle::ImagePair
Bundle::select_initial_pair (void)
{
    // TODO: Replace with proper implementation
    return ImagePair(0, 1);
}

int
Bundle::select_next_view (void)
{
    // TODO: Replace with proper implementation
    static int i = 2;
    return i++;
}

void
Bundle::compute_sift_descriptors (int view_id)
{
    Viewport& view = this->viewports[view_id];

    /* Compute SIFT descriptors. */
    sfm::Sift sift(this->options.sift_options);
    sift.set_image(view.image);
    sift.process();

    /* Convert descriptors to matching data structure. */
    sfm::Sift::Descriptors const& descr = sift.get_descriptors();
    view.descr_info.resize(descr.size());
    view.descr_data.allocate(descr.size() * 128);
    float* out_ptr = view.descr_data.begin();
    for (std::size_t i = 0; i < descr.size(); ++i, out_ptr += 128)
    {
        sfm::Sift::Descriptor const& d = descr[i];
        std::copy(d.data.begin(), d.data.end(), out_ptr);
        view.descr_info[i].pos[0] = d.x;
        view.descr_info[i].pos[1] = d.y;
        view.descr_info[i].feature2d_id = -1;
    }

    /* FIXME: How to enforce this properly? */
    this->options.sift_matching_options.descriptor_length = 128;
}

void
Bundle::two_view_pose (ImagePair* image_pair)
{
    Viewport const& view_1 = this->viewports[image_pair->first];
    Viewport const& view_2 = this->viewports[image_pair->second];

    /* Perform two-view descriptor matching. */
    MatchingResult matching_result;
    {
        util::WallTimer timer;
        sfm::match_features(this->options.sift_matching_options,
            view_1.descr_data.begin(), view_1.descr_info.size(),
            view_2.descr_data.begin(), view_2.descr_info.size(),
            &matching_result);
        sfm::remove_inconsistent_matches(&matching_result);
        std::cout << "Two-view matching took " << timer.get_elapsed() << "ms, "
            << sfm::count_consistent_matches(matching_result)
            << " matches." << std::endl;
    }

    /* Build correspondences from feature matching result. */
    sfm::Correspondences unfiltered_matches;
    CorrespondenceIndices unfiltered_indices;
    {
        std::vector<int> const& m12 = matching_result.matches_1_2;
        for (std::size_t i = 0; i < m12.size(); ++i)
        {
            if (m12[i] < 0)
                continue;

            sfm::Correspondence match;
            match.p1[0] = view_1.descr_info[i].pos[0];
            match.p1[1] = view_1.descr_info[i].pos[1];
            match.p2[0] = view_2.descr_info[m12[i]].pos[0];
            match.p2[1] = view_2.descr_info[m12[i]].pos[1];
            unfiltered_matches.push_back(match);
            unfiltered_indices.push_back(std::make_pair(i, m12[i]));
        }
    }

    /* Pose RANSAC. */
    sfm::PoseRansac2D2D::Result ransac_result;
    {
        sfm::PoseRansac2D2D ransac(this->options.ransac2d2d_options);
        util::WallTimer timer;
        ransac.estimate(unfiltered_matches, &ransac_result);
        std::cout << "RANSAC took " << timer.get_elapsed() << "ms, "
            << ransac_result.inliers.size() << " inliers." << std::endl;
    }

    /* Build correspondences from inliers only. */
    int const num_inliers = ransac_result.inliers.size();
    Correspondences inlier_matches(num_inliers);
    image_pair->indices.resize(num_inliers);
    for (int i = 0; i < num_inliers; ++i)
    {
        int const inlier_id = ransac_result.inliers[i];
        inlier_matches[i] = unfiltered_matches[inlier_id];
        image_pair->indices[i] = unfiltered_indices[inlier_id];
    }

    /* Find normalization for inliers and re-compute fundamental. */
    std::cout << "Re-computing fundamental matrix for inliers..." << std::endl;
    {
        math::Matrix3d T1, T2;
        sfm::FundamentalMatrix F;
        sfm::compute_normalization(inlier_matches, &T1, &T2);
        sfm::apply_normalization(T1, T2, &inlier_matches);
        sfm::fundamental_least_squares(inlier_matches, &F);
        sfm::enforce_fundamental_constraints(&F);
        image_pair->fundamental = T2.transposed() * F * T1;
    }
}

void
Bundle::triangulate_pair (ImagePair const& image_pair)
{
    Viewport const& view1 = this->viewports[image_pair.first];
    Viewport const& view2 = this->viewports[image_pair.second];

    /* Compute pose from fundamental matrix. */
    std::cout << "Computing pose..." << std::endl;
    sfm::CameraPose pose1, pose2;
    {
        /* Populate K-matrices. */
        int const width1 = view1.image->width();
        int const height1 = view1.image->height();
        int const width2 = view2.image->width();
        int const height2 = view2.image->height();
        double flen1 = view1.focal_length
            * static_cast<double>(std::max(width1, height1));
        double flen2 = view2.focal_length
            * static_cast<double>(std::max(width2, height2));
        pose1.set_k_matrix(flen1, width1 / 2.0, height1 / 2.0);
        pose1.init_canonical_form();
        pose2.set_k_matrix(flen2, width2 / 2.0, height2 / 2.0);

        /* Compute essential from fundamental. */
        sfm::EssentialMatrix E = pose2.K.transposed()
            * image_pair.fundamental * pose1.K;
        /* Compute pose from essential. */
        std::vector<sfm::CameraPose> poses;
        sfm::pose_from_essential(E, &poses);
        /* Find the correct pose using point test. */
        bool found_pose = false;
        sfm::Correspondence test_match;
        {
            int index_view1 = image_pair.indices[0].first;
            int index_view2 = image_pair.indices[0].second;
            test_match.p1[0] = view1.descr_info[index_view1].pos[0];
            test_match.p1[1] = view1.descr_info[index_view1].pos[1];
            test_match.p2[0] = view1.descr_info[index_view2].pos[0];
            test_match.p2[1] = view1.descr_info[index_view2].pos[1];
        }

        for (std::size_t i = 0; i < poses.size(); ++i)
        {
            poses[i].K = pose2.K;
            if (sfm::is_consistent_pose(test_match, pose1, poses[i]))
            {
                pose2 = poses[i];
                found_pose = true;
                break;
            }
        }
        if (!found_pose)
        {
            std::cout << "Could not find valid pose." << std::endl;
            return;
        }
    }

    /* Triangulate 3D points from pose. */
    // TODO: Add this to feature data structure
    std::cout << "Producing point model..." << std::endl;
    mve::TriangleMesh::Ptr mesh = mve::TriangleMesh::create();
    {
        for (std::size_t i = 0; i < image_pair.indices.size(); ++i)
        {
            int index1 = image_pair.indices[i].first;
            int index2 = image_pair.indices[i].second;
            Correspondence match;
            match.p1[0] = view1.descr_info[index1].pos[0];
            match.p1[1] = view1.descr_info[index1].pos[1];
            match.p2[0] = view2.descr_info[index2].pos[0];
            match.p2[1] = view2.descr_info[index2].pos[1];
            math::Vec3d x = sfm::triangulate_match(match, pose1, pose2);
            mesh->get_vertices().push_back(x);
        }
    }
    mve::geom::save_mesh(mesh, "/tmp/pose.ply");
}

SFM_NAMESPACE_END
