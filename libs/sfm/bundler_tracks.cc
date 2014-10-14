#include <iostream>
#include <set>

#include "mve/image_tools.h"
#include "mve/image_drawing.h"
#include "sfm/bundler_tracks.h"

SFM_NAMESPACE_BEGIN
SFM_BUNDLER_NAMESPACE_BEGIN

void
Tracks::compute (PairwiseMatching const& matching,
    ViewportList* viewports, TrackList* tracks)
{
    /* Initialize per-viewport track IDs. */
    for (std::size_t i = 0; i < viewports->size(); ++i)
    {
        Viewport& viewport = viewports->at(i);
        viewport.track_ids.resize(viewport.features.positions.size(), -1);
    }

    /* Propagate track IDs. */
    if (this->opts.verbose_output)
        std::cout << "Propagating track IDs..." << std::endl;

    std::size_t track_counter = 0;
    std::size_t num_conflicts = 0;

    /* Iterate over all pairwise matchings. */
    for (std::size_t i = 0; i < matching.size(); ++i)
    {
        TwoViewMatching const& tvm = matching[i];
        Viewport& viewport1 = viewports->at(tvm.view_1_id);
        Viewport& viewport2 = viewports->at(tvm.view_2_id);

        /* Iterate over matches for a pair. */
        for (std::size_t j = 0; j < tvm.matches.size(); ++j)
        {
            CorrespondenceIndex idx = tvm.matches[j];
            if (viewport1.track_ids[idx.first] == -1
                && viewport2.track_ids[idx.second] == -1)
            {
                /* No track ID associated with the match. Create track. */
                viewport1.track_ids[idx.first] = track_counter;
                viewport2.track_ids[idx.second] = track_counter;
                track_counter += 1;
            }
            else if (viewport1.track_ids[idx.first] == -1
                && viewport2.track_ids[idx.second] != -1)
            {
                /* Propagate track ID from first to second view. */
                viewport1.track_ids[idx.first]
                    = viewport2.track_ids[idx.second];
            }
            else if (viewport1.track_ids[idx.first] != -1
                && viewport2.track_ids[idx.second] == -1)
            {
                /* Propagate track ID from second to first view. */
                viewport2.track_ids[idx.second]
                    = viewport1.track_ids[idx.first];
            }
            else if (viewport1.track_ids[idx.first]
                == viewport2.track_ids[idx.second])
            {
                /* Track ID already propagated. */
            }
            else
            {
                /*
                 * A track ID is already associated with both ends of a match,
                 * however, is not consistent.
                 */
                num_conflicts += 1;
            }
        }
    }

    std::cerr << "Warning: " << num_conflicts
        << " conflicts while propagating track IDs." << std::endl;

    /* Create tracks. */
    tracks->clear();
    tracks->resize(track_counter);
    for (std::size_t i = 0; i < viewports->size(); ++i)
    {
        Viewport const& viewport = viewports->at(i);
        for (std::size_t j = 0; j < viewport.track_ids.size(); ++j)
        {
            int const track_id = viewport.track_ids[j];
            if (track_id < 0)
                continue;
            FeatureReference feature_ref;
            feature_ref.view_id = i;
            feature_ref.feature_id = j;
            tracks->at(track_id).features.push_back(feature_ref);
        }
    }

    /* Find and remove tracks with conflicts. */
    if (this->opts.verbose_output)
        std::cout << "Removing tracks with conflicts..." << std::flush;
    std::size_t const num_total_tracks = tracks->size();
    std::size_t const num_invalid_tracks
        = this->remove_invalid_tracks(viewports, tracks);
    if (this->opts.verbose_output)
        std::cout << " deleted " << num_invalid_tracks << " of "
            << num_total_tracks << " tracks." << std::endl;

    /* Compute color for every track. */
    if (this->opts.verbose_output)
        std::cout << "Colorizing tracks..." << std::endl;
    for (std::size_t i = 0; i < tracks->size(); ++i)
    {
        Track& track = tracks->at(i);
        math::Vec4f color(0.0f, 0.0f, 0.0f, 0.0f);
        for (std::size_t j = 0; j < track.features.size(); ++j)
        {
            FeatureReference const& ref = track.features[j];
            FeatureSet const& features = viewports->at(ref.view_id).features;
            math::Vec3f const feature_color(features.colors[ref.feature_id]);
            color += math::Vec4f(feature_color, 1.0f);
        }
        track.color[0] = static_cast<uint8_t>(color[0] / color[3] + 0.5f);
        track.color[1] = static_cast<uint8_t>(color[1] / color[3] + 0.5f);
        track.color[2] = static_cast<uint8_t>(color[2] / color[3] + 0.5f);
    }
}

/* ---------------------------------------------------------------- */

int
Tracks::remove_invalid_tracks (ViewportList* viewports, TrackList* tracks)
{
    /*
     * Detect invalid tracks where a track contains no features, or
     * multiple features from a single view.
     */
    std::vector<bool> delete_tracks(tracks->size());
    std::size_t num_invalid_tracks = 0;
    for (std::size_t i = 0; i < tracks->size(); ++i)
    {
        if (tracks->at(i).features.empty())
        {
            num_invalid_tracks += 1;
            delete_tracks[i] = true;
            continue;
        }

        std::set<int> view_ids;
        for (std::size_t j = 0; j < tracks->at(i).features.size(); ++j)
        {
            FeatureReference const& ref = tracks->at(i).features[j];
            if (view_ids.insert(ref.view_id).second == false)
            {
                num_invalid_tracks += 1;
                delete_tracks[i] = true;
                break;
            }
        }
    }

    /* Create a mapping from old to new track IDs. */
    std::vector<int> id_mapping(delete_tracks.size(), -1);
    int valid_track_counter = 0;
    for (std::size_t i = 0; i < delete_tracks.size(); ++i)
    {
        if (delete_tracks[i])
            continue;
        id_mapping[i] = valid_track_counter;
        valid_track_counter += 1;
    }

    /* Fix track IDs stored in the viewports. */
    for (std::size_t i = 0; i < viewports->size(); ++i)
    {
        std::vector<int>& track_ids = viewports->at(i).track_ids;
        for (std::size_t j = 0; j < track_ids.size(); ++j)
            if (track_ids[j] >= 0)
                track_ids[j] = id_mapping[track_ids[j]];
    }

    /* Clean the tracks from the vector. */
    math::algo::vector_clean(delete_tracks, tracks);

    return num_invalid_tracks;
}

/* ---------------------------------------------------------------- */

#if 0
    std::cout << std::endl << "Debugging tracks:" << std::endl;
    for (std::size_t i = 0; i < track_list.size(); ++i)
    {
        std::cout << "Track " << i << ":";
        for (std::size_t j = 0; j < track_list[i].features.size(); ++j)
            std::cout << " (" << track_list[i].features[j].view_id
                << "," << track_list[i].features[j].feature_id << ")";
        std::cout << std::endl;
    }

    std::cout << std::endl << "Debugging viewports:" << std::endl;
    for (std::size_t i = 0; i < viewports.size(); ++i)
    {
        std::cout << "View " << i << ":";
        for (std::size_t j = 0; j < viewports[i].track_ids.size(); ++j)
            std::cout << " " << viewports[i].track_ids[j];
        std::cout << std::endl;
    }
#endif

/* ---------------------------------------------------------------- */

#define WITH_MATCHES 1

struct ImageInfo
{
    mve::ByteImage::Ptr image;
    int view_id;
    int feature_id;
    float feature_x;
    float feature_y;
    int start_x;
    int start_y;
#if WITH_MATCHES
    std::vector<std::pair<int, int> > matching;
#endif
};

mve::ByteImage::Ptr
visualize_track (Track const& track, ViewportList const& viewports,
    mve::Scene::Ptr scene, std::string const& image_embedding,
    PairwiseMatching const& pairwise_matching)
{
    std::cout << "DEBUG: Track with " << track.features.size()
        << " features." << std::endl;

    int const max_width = 400;
    int const max_height = 260;
    int const margin = 5;

    /* Fill image information. */
    std::vector<ImageInfo> images;
    images.resize(track.features.size());
    for (std::size_t i = 0; i < track.features.size(); ++i)
    {
        int const view_id = track.features[i].view_id;
        int const feature_id = track.features[i].feature_id;

        Viewport const& viewport = viewports[view_id];
        mve::View::Ptr view = scene->get_view_by_id(view_id);
        mve::ByteImage::Ptr image = view->get_byte_image(image_embedding);
        if (image == NULL)
            throw std::runtime_error("Cannot find image");
        if (image->channels() != 3)
            throw std::runtime_error("Unsupported number of channels");

        /* Resize image to match feature extraction size. */
        while (image->width() > viewport.width
            && image->height() > viewport.height)
            image = mve::image::rescale_half_size<uint8_t>(image);
        if (image->width() != viewport.width
            || image->height() != viewport.height)
            throw std::runtime_error("Image dimensions do not match");

        ImageInfo& info = images[i];
        info.image = image;
        info.feature_x = viewport.features.positions[feature_id][0];
        info.feature_y = viewport.features.positions[feature_id][1];
        info.view_id = view_id;
        info.feature_id = feature_id;

#if WITH_MATCHES
        for (std::size_t j = 0; j < pairwise_matching.size(); ++j)
        {
            TwoViewMatching const& match = pairwise_matching[j];
            if (match.view_1_id == view_id)
                for (std::size_t k = 0; k < match.matches.size(); ++k)
                    if (match.matches[k].first == feature_id)
                        info.matching.push_back(std::pair<int, int>
                            (match.view_2_id, match.matches[k].second));
            if (match.view_2_id == view_id)
                for (std::size_t k = 0; k < match.matches.size(); ++k)
                    if (match.matches[k].second == feature_id)
                        info.matching.push_back(std::pair<int, int>
                            (match.view_1_id, match.matches[k].first));
        }
#endif
    }

    int num_cols = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(track.features.size()))));
    mve::ByteImage::Ptr image = mve::ByteImage::create(
        num_cols * max_width + (num_cols - 1) * margin,
        num_cols * max_height + (num_cols - 1) * margin, 3);
    std::cout << "DEBUG: Output image dimensions: "
        << image->width() << "x" << image->height() << std::endl;

    /* Resize all images to max width/height. */
    for (std::size_t i = 0; i < images.size(); ++i)
    {
        int const col = i % num_cols;
        int const row = i / num_cols;

        ImageInfo& info = images[i];
        while (info.image->width() > max_width
            || info.image->height() > max_height)
        {
            info.image = mve::image::rescale_half_size<uint8_t>(info.image);
            info.feature_x = (info.feature_x + 0.5f) / 2.0f - 0.5f;
            info.feature_y = (info.feature_y + 0.5f) / 2.0f - 0.5f;
        }
        std::cout << "DEBUG: Rescaled view " << info.view_id << " to "
            << info.image->width() << "x" << info.image->height() << std::endl;

        info.start_x = col * (max_width + margin);
        info.start_y = row * (max_height + margin);

        int rowstride = info.image->width() * info.image->channels();
        for (int y = 0; y < info.image->height(); ++y)
            std::copy(info.image->begin() + y * rowstride,
                info.image->begin() + (y + 1) * rowstride,
                image->begin() + info.start_x * image->channels()
                + (info.start_y + y) * image->width() * image->channels());
    }

    /* Draw circles for each feature. */
    for (std::size_t i = 0; i < track.features.size(); ++i)
    {
        uint8_t color[3] = { 255, 0, 0};
        mve::image::draw_circle(*image,
            images[i].start_x + images[i].feature_x,
            images[i].start_y + images[i].feature_y,
            5, color);
    }

#if WITH_MATCHES
    /* Draw lines between the features. */
    for (std::size_t i = 0; i < images.size(); ++i)
    {
        uint8_t color[3] = { 0, 255, 0};
        std::vector<std::pair<int, int> > const& matching = images[i].matching;
        for (std::size_t j = 0; j < matching.size(); ++j)
        {
            int view_id = matching[j].first;
            int feature_id = matching[j].second;

            for (std::size_t k = 0; k < images.size(); ++k)
                if (view_id == images[k].view_id && feature_id == images[k].feature_id)
                    mve::image::draw_line(*image,
                        images[i].start_x + images[i].feature_x,
                        images[i].start_y + images[i].feature_y,
                        images[k].start_x + images[k].feature_x,
                        images[k].start_y + images[k].feature_y,
                        color);
        }
    }
#endif

    return image;
}

SFM_BUNDLER_NAMESPACE_END
SFM_NAMESPACE_END
