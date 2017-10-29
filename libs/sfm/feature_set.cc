/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <iostream>
#include <algorithm>

#include "sfm/feature_set.h"

SFM_NAMESPACE_BEGIN

namespace
{
    template <typename T>
    bool
    compare_scale (T const& descr1, T const& descr2)
    {
        return descr1.scale > descr2.scale;
    }
}  /* namespace */

void
FeatureSet::compute_features (mve::ByteImage::Ptr image)
{
    this->colors.clear();
    this->positions.clear();
    this->width = image->width();
    this->height = image->height();

    /* Make sure these are in the right order. Matching relies on it. */
    if (this->opts.feature_types & FEATURE_SIFT)
        this->compute_sift(image);
    if (this->opts.feature_types & FEATURE_SURF)
        this->compute_surf(image);
}

void
FeatureSet::normalize_feature_positions (void)
{
    /* Normalize image coordinates. */
    float const fwidth = static_cast<float>(this->width);
    float const fheight = static_cast<float>(this->height);
    float const fnorm = std::max(fwidth, fheight);
    for (std::size_t i = 0; i < this->positions.size(); ++i)
    {
        math::Vec2f& pos = this->positions[i];
        pos[0] = (pos[0] + 0.5f - fwidth / 2.0f) / fnorm;
        pos[1] = (pos[1] + 0.5f - fheight / 2.0f) / fnorm;
    }
}

void
FeatureSet::compute_sift (mve::ByteImage::ConstPtr image)
{
    /* Compute features. */
    Sift::Descriptors descr;
    {
        Sift sift(this->opts.sift_opts);
        sift.set_image(image);
        sift.process();
        descr = sift.get_descriptors();
    }

    /* Sort features by scale for low-res matching. */
    std::sort(descr.begin(), descr.end(), compare_scale<sfm::Sift::Descriptor>);

    /* Prepare and copy to data structures. */
    std::size_t offset = this->positions.size();
    this->positions.resize(offset + descr.size());
    this->colors.resize(offset + descr.size());

    for (std::size_t i = 0; i < descr.size(); ++i)
    {
        Sift::Descriptor const& d = descr[i];
        this->positions[offset + i] = math::Vec2f(d.x, d.y);
        image->linear_at(d.x, d.y, this->colors[offset + i].begin());
    }

    /* Keep SIFT descriptors. */
    std::swap(descr, this->sift_descriptors);
}

void
FeatureSet::compute_surf (mve::ByteImage::ConstPtr image)
{
    /* Compute features. */
    Surf::Descriptors descr;
    {
        Surf surf(this->opts.surf_opts);
        surf.set_image(image);
        surf.process();
        descr = surf.get_descriptors();
    }

    /* Sort features by scale for low-res matching. */
    std::sort(descr.begin(), descr.end(), compare_scale<sfm::Surf::Descriptor>);

    /* Prepare and copy to data structures. */
    std::size_t offset = this->positions.size();
    this->positions.resize(offset + descr.size());
    this->colors.resize(offset + descr.size());

    for (std::size_t i = 0; i < descr.size(); ++i)
    {
        Surf::Descriptor const& d = descr[i];
        this->positions[offset + i] = math::Vec2f(d.x, d.y);
        image->linear_at(d.x, d.y, this->colors[offset + i].begin());
    }

    /* Keep SURF descriptors. */
    std::swap(descr, this->surf_descriptors);
}

void
FeatureSet::clear_descriptors (void)
{
    this->sift_descriptors.clear();
    this->sift_descriptors.shrink_to_fit();
    this->surf_descriptors.clear();
    this->surf_descriptors.shrink_to_fit();
}

SFM_NAMESPACE_END
