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
    /* Mark all view ports as remaining (to be bundled). */
    for (std::size_t i = 0; i < this->viewports.size(); ++i)
        this->remaining.insert(i);

    /* Bundle initial pair, which is assumed to be known here. */
    ImagePair initial_pair = this->select_initial_pair();
    this->compute_sift_descriptors(initial_pair.first);
    this->compute_sift_descriptors(initial_pair.second);
    this->two_view_pose(&initial_pair);
    this->triangulate_initial_pair(initial_pair);
    this->remaining.erase(initial_pair.first);
    this->remaining.erase(initial_pair.second);

    std::cout << "Saving tracks after initial pair..." << std::endl;
    this->save_tracks_to_mesh("/tmp/initialpair.ply");

    while (this->remaining.empty())
    {
        /* Obtain next view to reconstruct. */
        std::size_t view_id = this->select_next_view();
        this->remaining.erase(view_id);

        /* Compute descriptors for this view. */
        this->compute_sift_descriptors(view_id);

        /*
         * Create a map with (track IDs, SIFT IDs) to map 3D points to
         * 2D points in this image. If the same track maps to multiple
         * SIFT IDs, the mapping is invalidated by setting the SIFT ID to -1.
         */
        std::map<int, int> tracks_to_sift;

        for (std::size_t i = 0; i < this->viewports.size(); ++i)
        {
            if (this->viewports[i].descr_info.empty())
                continue;

            /* Perform two-view matching to filter feature matches. */
            std::cout << "Processing image pair " << view_id
                << "," << i << "..." << std::endl;
            Bundle::ImagePair image_pair(view_id, i);
            this->two_view_pose(&image_pair);

            if (image_pair.indices.size() < 8)
            {
                std::cout << "Skipping pair with " << image_pair.indices.size()
                    << " correspondences." << std::endl;
                continue;
            }

            /* Collect 2D-3D correspondences for view. */
            for (std::size_t j = 0; j < image_pair.indices.size(); ++j)
            {
                int this_sift_id = image_pair.indices[i].first;
                int other_sift_id = image_pair.indices[i].second;
                int other_f2d_id = this->viewports[i].descr_info[other_sift_id].feature2d_id;
                if (other_f2d_id < 0)
                    continue;
                int other_f3d_id = this->features[other_f2d_id].feature3d_id;



                // TODO CONTINUE!



            }
        }
    }
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
    return *this->remaining.begin();
}

void
Bundle::compute_sift_descriptors (std::size_t view_id)
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
    Matching::Result matching_result;
    {
        util::WallTimer timer;
        sfm::Matching::twoway_match(this->options.sift_matching_options,
            view_1.descr_data.begin(), view_1.descr_info.size(),
            view_2.descr_data.begin(), view_2.descr_info.size(),
            &matching_result);
        sfm::Matching::remove_inconsistent_matches(&matching_result);
        std::cout << "Two-view matching took " << timer.get_elapsed() << "ms, "
            << sfm::Matching::count_consistent_matches(matching_result)
            << " matches." << std::endl;
    }

    /* Require at least 8 matches. (This can be much higher?) */
    if (matching_result.matches_1_2.size() < 8)
        return;

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

    /* Require at least 8 inlier matches. */
    if (matching_result.matches_1_2.size() < 8)
        return;

    /* Build correspondences from inliers only. */
    int const num_inliers = ransac_result.inliers.size();
    sfm::Correspondences inlier_matches(num_inliers);
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
Bundle::triangulate_initial_pair (ImagePair const& image_pair)
{
    Viewport& view1 = this->viewports[image_pair.first];
    Viewport& view2 = this->viewports[image_pair.second];

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
    this->features.reserve(image_pair.indices.size() * 2);
    this->tracks.reserve(image_pair.indices.size());
    for (std::size_t i = 0; i < image_pair.indices.size(); ++i)
    {
        int index1 = image_pair.indices[i].first;
        int index2 = image_pair.indices[i].second;
        Feature2DRef& f2dr_1 = view1.descr_info[index1];
        Feature2DRef& f2dr_2 = view2.descr_info[index2];

        /* Triangulate match. */
        Correspondence match;
        std::copy(f2dr_1.pos, f2dr_1.pos + 2, match.p1);
        std::copy(f2dr_2.pos, f2dr_2.pos + 2, match.p2);
        math::Vec3d x = sfm::triangulate_match(match, pose1, pose2);

        /* Register new features. */
        Feature2D f2d_1, f2d_2;
        std::copy(f2dr_1.pos, f2dr_1.pos + 2, f2d_1.pos);
        std::copy(f2dr_2.pos, f2dr_2.pos + 2, f2d_2.pos);
        f2dr_1.feature2d_id = this->features.size();
        this->features.push_back(f2d_1);
        f2dr_2.feature2d_id = this->features.size();
        this->features.push_back(f2d_2);

        /* Register new track. */
        Feature3D f3d;
        std::copy(*x, *x + 3, f3d.pos);
        f3d.feature2d_ids.push_back(f2dr_1.feature2d_id);
        f3d.feature2d_ids.push_back(f2dr_2.feature2d_id);
        this->tracks.push_back(f3d);
    }
}

void
Bundle::save_tracks_to_mesh(std::string const& filename)
{
    mve::TriangleMesh::Ptr mesh = mve::TriangleMesh::create();
    mve::TriangleMesh::VertexList& verts = mesh->get_vertices();
    for (std::size_t i = 0; i < this->tracks.size(); ++i)
    {
        math::Vec3d x(this->tracks[i].pos);
        verts.push_back(x);
    }
    mve::geom::save_mesh(mesh, filename);
}

SFM_NAMESPACE_END
