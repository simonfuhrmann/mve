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
#include <fstream>
#include <limits>
#include <sstream>
#include <vector>
#include <cstring>
#include <cerrno>

#include "util/exception.h"
#include "sfm/bundler_common.h"

#define PREBUNDLE_SIGNATURE "MVE_PREBUNDLE\n"
#define PREBUNDLE_SIGNATURE_LEN 14

#define SURVEY_SIGNATURE "MVE_SURVEY\n"
#define SURVEY_SIGNATURE_LEN 11

SFM_NAMESPACE_BEGIN
SFM_BUNDLER_NAMESPACE_BEGIN


/* --------------- Data Structure for Feature Tracks -------------- */

void
Track::invalidate (void)
{
    std::fill(this->pos.begin(), this->pos.end(),
        std::numeric_limits<float>::quiet_NaN());
}

void
Track::remove_view (int view_id)
{
    for (FeatureReferenceList::iterator iter = this->features.begin();
        iter != this->features.end();)
    {
        if (iter->view_id == view_id)
            iter = this->features.erase(iter);
        else
            iter++;
    }
}

/* ------------------ Input/Output for Prebundle ------------------ */

void
save_prebundle_data (ViewportList const& viewports,
    PairwiseMatching const& matching, std::ostream& out)
{
    /* Write signature. */
    out.write(PREBUNDLE_SIGNATURE, PREBUNDLE_SIGNATURE_LEN);

    /* Write number of viewports. */
    int32_t num_viewports = static_cast<int32_t>(viewports.size());
    out.write(reinterpret_cast<char const*>(&num_viewports), sizeof(int32_t));

    /* Write per-viewport data. */
    for (std::size_t i = 0; i < viewports.size(); ++i)
    {
        FeatureSet const& vpf = viewports[i].features;

        /* Write positions. */
        int32_t num_positions = static_cast<int32_t>(vpf.positions.size());
        out.write(reinterpret_cast<char const*>(&num_positions), sizeof(int32_t));
        for (std::size_t j = 0; j < vpf.positions.size(); ++j)
            out.write(reinterpret_cast<char const*>(&vpf.positions[j]), sizeof(math::Vec2f));

        /* Write colors. */
        int32_t num_colors = static_cast<int32_t>(vpf.colors.size());
        out.write(reinterpret_cast<char const*>(&num_colors), sizeof(int32_t));
        for (std::size_t j = 0; j < vpf.colors.size(); ++j)
            out.write(reinterpret_cast<char const*>(&vpf.colors[j]), sizeof(math::Vec3uc));
    }

    /* Write number of matching pairs. */
    int32_t num_pairs = static_cast<int32_t>(matching.size());
    out.write(reinterpret_cast<char const*>(&num_pairs), sizeof(int32_t));

    /* Write per-matching pair data. */
    for (std::size_t i = 0; i < matching.size(); ++i)
    {
        TwoViewMatching const& tvr = matching[i];
        int32_t id1 = static_cast<int32_t>(tvr.view_1_id);
        int32_t id2 = static_cast<int32_t>(tvr.view_2_id);
        int32_t num_matches = static_cast<int32_t>(tvr.matches.size());
        out.write(reinterpret_cast<char const*>(&id1), sizeof(int32_t));
        out.write(reinterpret_cast<char const*>(&id2), sizeof(int32_t));
        out.write(reinterpret_cast<char const*>(&num_matches), sizeof(int32_t));
        for (std::size_t j = 0; j < tvr.matches.size(); ++j)
        {
            CorrespondenceIndex const& c = tvr.matches[j];
            int32_t i1 = static_cast<int32_t>(c.first);
            int32_t i2 = static_cast<int32_t>(c.second);
            out.write(reinterpret_cast<char const*>(&i1), sizeof(int32_t));
            out.write(reinterpret_cast<char const*>(&i2), sizeof(int32_t));
        }
    }
}

void
load_prebundle_data (std::istream& in, ViewportList* viewports,
    PairwiseMatching* matching)
{
    /* Read and check file signature. */
    char signature[PREBUNDLE_SIGNATURE_LEN + 1];
    in.read(signature, PREBUNDLE_SIGNATURE_LEN);
    signature[PREBUNDLE_SIGNATURE_LEN] = '\0';
    if (std::string(PREBUNDLE_SIGNATURE) != signature)
        throw std::invalid_argument("Invalid prebundle file signature");

    viewports->clear();
    matching->clear();

    /* Read number of viewports. */
    int32_t num_viewports;
    in.read(reinterpret_cast<char*>(&num_viewports), sizeof(int32_t));
    viewports->resize(num_viewports);

    /* Read per-viewport data. */
    for (int i = 0; i < num_viewports; ++i)
    {
        FeatureSet& vpf = viewports->at(i).features;

        /* Read positions. */
        int32_t num_positions;
        in.read(reinterpret_cast<char*>(&num_positions), sizeof(int32_t));
        vpf.positions.resize(num_positions);
        for (int j = 0; j < num_positions; ++j)
            in.read(reinterpret_cast<char*>(&vpf.positions[j]), sizeof(math::Vec2f));

        /* Read colors. */
        int32_t num_colors;
        in.read(reinterpret_cast<char*>(&num_colors), sizeof(int32_t));
        vpf.colors.resize(num_colors);
        for (int j = 0; j < num_colors; ++j)
            in.read(reinterpret_cast<char*>(&vpf.colors[j]), sizeof(math::Vec3uc));
    }

    /* Read number of matching pairs. */
    int32_t num_pairs;
    in.read(reinterpret_cast<char*>(&num_pairs), sizeof(int32_t));

    /* Read per-matching pair data. */
    for (int32_t i = 0; i < num_pairs; ++i)
    {
        int32_t id1, id2, num_matches;
        in.read(reinterpret_cast<char*>(&id1), sizeof(int32_t));
        in.read(reinterpret_cast<char*>(&id2), sizeof(int32_t));
        in.read(reinterpret_cast<char*>(&num_matches), sizeof(int32_t));

        TwoViewMatching tvr;
        tvr.view_1_id = static_cast<int>(id1);
        tvr.view_2_id = static_cast<int>(id2);
        tvr.matches.reserve(num_matches);
        for (int32_t j = 0; j < num_matches; ++j)
        {
            int32_t i1, i2;
            in.read(reinterpret_cast<char*>(&i1), sizeof(int32_t));
            in.read(reinterpret_cast<char*>(&i2), sizeof(int32_t));
            CorrespondenceIndex c;
            c.first = static_cast<int>(i1);
            c.second = static_cast<int>(i2);
            tvr.matches.push_back(c);
        }
        matching->push_back(tvr);
    }
}

void
save_prebundle_to_file (ViewportList const& viewports,
    PairwiseMatching const& matching, std::string const& filename)
{
    std::ofstream out(filename.c_str(), std::ios::binary);
    if (!out.good())
        throw util::FileException(filename, std::strerror(errno));
    save_prebundle_data(viewports, matching, out);
    out.close();
}

void
load_prebundle_from_file (std::string const& filename,
    ViewportList* viewports, PairwiseMatching* matching)
{
    std::ifstream in(filename.c_str(), std::ios::binary);
    if (!in.good())
        throw util::FileException(filename, std::strerror(errno));

    try
    {
        load_prebundle_data(in, viewports, matching);
    }
    catch (...)
    {
        in.close();
        throw;
    }

    if (in.eof())
    {
        in.close();
        throw util::Exception("Premature EOF");
    }
    in.close();
}

void
load_survey_from_file (std::string const& filename,
    SurveyPointList* survey_points)
{
    std::ifstream in(filename.c_str());
    if (!in.good())
        throw util::FileException(filename, std::strerror(errno));

    char signature[SURVEY_SIGNATURE_LEN + 1];
    in.read(signature, SURVEY_SIGNATURE_LEN);
    signature[SURVEY_SIGNATURE_LEN] = '\0';
    if (std::string(SURVEY_SIGNATURE) != signature)
        throw std::invalid_argument("Invalid survey file signature");

    std::size_t num_points = 0;
    std::size_t num_observations = 0;
    in >> num_points >> num_observations;
    if (in.fail())
    {
        in.close();
        throw util::Exception("Invalid survey file header");
    }

    survey_points->resize(num_points);
    for (std::size_t i = 0; i < num_points; ++i)
    {
        SurveyPoint& survey_point = survey_points->at(i);
        for (int j = 0; j < 3; ++j)
            in >> survey_point.pos[j];
    }

    for (std::size_t i = 0; i < num_observations; ++i)
    {
        int point_id, view_id;
        float x, y;
        in >> point_id >> view_id >> x >> y;

        if (static_cast<std::size_t>(point_id) > num_points)
            throw util::Exception("Invalid survey point id");

        SurveyPoint& survey_point = survey_points->at(point_id);
        survey_point.observations.emplace_back(view_id, x, y);
    }

    if (in.fail())
    {
        in.close();
        throw util::Exception("Parsing error");
    }

    in.close();
}

/* ---------------------- Feature undistortion -------------------- */

namespace {
    double distort_squared_radius (double const r2, double const k1,
        double const k2)
    {
        // Compute the distorted squared radius:
        // (radius * distortion_coeff)^2 = r^2 * coeff^2
        double coeff = 1.0 + r2 * k1 + r2 * r2 * k2;
        return r2 * coeff * coeff;
    }

    double solve_undistorted_squared_radius (double const r2,
        double const k1, double const k2)
    {
        // Guess initial interval upper and lower bound
        double lbound = r2, ubound = r2;
        while (distort_squared_radius(lbound, k1, k2) > r2)
        {
            ubound = lbound;
            lbound /= 1.05;
        }
        while (distort_squared_radius(ubound, k1, k2) < r2)
        {
            lbound = ubound;
            ubound *= 1.05;
        }

        // We use bisection as we can easily find a pretty good initial guess,
        // and we don't want to compute all roots for a 5th degree polynomial.

        // Perform a bisection until epsilon accuracy is reached
        double mid = 0.5 * (lbound + ubound);
        while (mid != lbound && mid != ubound)
        {
            if (distort_squared_radius(mid, k1, k2) > r2)
                ubound = mid;
            else
                lbound = mid;
            mid = 0.5 * (lbound + ubound);
        }
        // Return center of interval
        return mid;
    }
}

math::Vec2f
undistort_feature (math::Vec2f const& f, double const k1, double const k2,
    float const focal_length)
{
    // Convert to camera coords
    double const r2 = f.square_norm() / MATH_POW2(focal_length);
    double scale = 1.0;
    if (r2 > 0.0)
        scale = std::sqrt(solve_undistorted_squared_radius(r2, k1, k2) / r2);
    return f * scale;
}

SFM_BUNDLER_NAMESPACE_END
SFM_NAMESPACE_END
