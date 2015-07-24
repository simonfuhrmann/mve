/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include "math/defines.h"
#include "math/functions.h"
#include "dmrecon/local_view_selection.h"
#include "dmrecon/mvs_tools.h"
#include "dmrecon/patch_sampler.h"
#include "dmrecon/settings.h"

MVS_NAMESPACE_BEGIN

LocalViewSelection::LocalViewSelection(
    std::vector<SingleView::Ptr> const& views,
    Settings const& settings,
    IndexSet const& globalViewIDs,
    IndexSet const& propagated,
    PatchSampler::Ptr sampler)
    :
    ViewSelection(settings),
    success(false),
    views(views),
    sampler(sampler)
{
    // inherited attribute
    this->selected = propagated;

    if (!sampler->success[settings.refViewNr]) {
        return;
    }
    if (selected.size() == settings.nrReconNeighbors)
        success = true;
    else if (selected.size() > settings.nrReconNeighbors) {
        std::cerr << "ERROR: Too many local neighbors propagated!" << std::endl;
        selected.clear();
    }

    available.clear();
    available.resize(views.size(), false);
    IndexSet::const_iterator id;
    for (id = globalViewIDs.begin(); id != globalViewIDs.end(); ++id) {
        available[*id] = true;
    }
    IndexSet::const_iterator sel;
    for (sel = selected.begin(); sel != selected.end(); ++sel) {
        available[*sel] = false;
    }
}

void
LocalViewSelection::performVS()
{
    if (selected.size() == settings.nrReconNeighbors) {
        success = true;
        return;
    }
    SingleView::Ptr refV = views[settings.refViewNr];

    math::Vec3f p(sampler->getMidWorldPoint());
    // pixel print in reference view
    float mfp = refV->footPrintScaled(p);
    math::Vec3f refDir = (p - refV->camPos).normalized();
    std::map<std::size_t, math::Vec3f> viewDir;
    std::map<std::size_t, math::Vec3f> epipolarPlane; // plane normal
    std::map<std::size_t, float> ncc;

    for (std::size_t i = 0; i < views.size(); ++i) {
        if (!available[i])
            continue;
        float tmpNCC = sampler->getFastNCC(i);
        assert(!MATH_ISNAN(tmpNCC));
        if (tmpNCC < settings.minNCC) {
            available[i] = false;
            continue;
        }
        ncc[i] = tmpNCC;
        viewDir[i] = (p - views[i]->camPos).normalized();
        epipolarPlane[i] = (viewDir[i].cross(refDir)).normalized();
    }
    IndexSet::const_iterator sel;
    for (sel = selected.begin(); sel != selected.end(); ++sel) {
        viewDir[*sel] = (p - views[*sel]->camPos).normalized();
        epipolarPlane[*sel] = (viewDir[*sel].cross(refDir)).normalized();
    }

    bool foundOne = true;
    while (selected.size() < settings.nrReconNeighbors && foundOne)
    {
        foundOne = false;
        std::size_t maxView = 0;
        float maxScore = 0.f;
        for (std::size_t i = 0; i < views.size(); ++i) {
            if (!available[i])
                continue;
            float score = ncc[i];

            // resolution difference
            float nfp = views[i]->footPrint(p);
            if (mfp / nfp < 0.5f) {
                score *= 0.01f;
            }

            // parallax w.r.t. reference view
            float dp = math::clamp(refDir.dot(viewDir[i]), -1.f, 1.f);
            float plx = std::acos(dp) * 180.f / pi;
            score *= parallaxToWeight(plx);
            assert(!MATH_ISNAN(score));

            for (sel = selected.begin(); sel != selected.end(); ++sel) {
                // parallax w.r.t. other selected views
                dp = math::clamp(viewDir[*sel].dot(viewDir[i]), -1.f, 1.f);
                plx = std::acos(dp) * 180.f / pi;
                score *= parallaxToWeight(plx);

                // epipolar geometry
                dp = epipolarPlane[i].dot(epipolarPlane[*sel]);
                dp = math::clamp(dp, -1.f, 1.f);
                float angle = std::abs(std::acos(dp) * 180.f / pi);
                if (angle > 90.f)
                    angle = 180.f - angle;

                angle = std::max(angle, 1.f);
                if (angle < settings.minParallax)
                    score *= angle / settings.minParallax;
                assert(!MATH_ISNAN(score));
            }
            if (score > maxScore) {
                foundOne = true;
                maxScore = score;
                maxView = i;
            }
        }
        if (foundOne) {
            selected.insert(maxView);
            available[maxView] = false;
        }
    }
    if (selected.size() == settings.nrReconNeighbors) {
        success = true;
    }
}

void
LocalViewSelection::replaceViews(IndexSet const & toBeReplaced)
{
    IndexSet::const_iterator tbr = toBeReplaced.begin();
    while (tbr != toBeReplaced.end()) {
        available[*tbr] = false;
        selected.erase(*tbr);
        ++tbr;
    }
    success = false;
    performVS();
}


MVS_NAMESPACE_END
