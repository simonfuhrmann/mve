/*
 * Copyright (C) 2015, Ronny Klowsky, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include "math/vector.h"
#include "dmrecon/global_view_selection.h"
#include "dmrecon/mvs_tools.h"
#include "dmrecon/settings.h"

MVS_NAMESPACE_BEGIN

GlobalViewSelection::GlobalViewSelection(
    std::vector<SingleView::Ptr> const& views,
    mve::Bundle::Features const& features,
    Settings const& settings)
    : ViewSelection(settings)
    , views(views)
    , features(features)
{
    available.clear();
    available.resize(views.size(), true);
    available[settings.refViewNr] = false;
    for (std::size_t i = 0; i < views.size(); ++i)
        if (views[i] == nullptr)
            available[i] = false;
}

void
GlobalViewSelection::performVS()
{
    selected.clear();
    bool foundOne = true;
    while (foundOne && (selected.size() < settings.globalVSMax))
    {
        float maxBenefit = 0.f;
        std::size_t maxView = 0;
        foundOne = false;
        for (std::size_t i = 0; i < views.size(); ++i)
        {
            if (!available[i])
                continue;

            float benefit = benefitFromView(i);
            if (benefit > maxBenefit) {
                maxBenefit = benefit;
                maxView = i;
                foundOne = true;
            }
        }
        if (foundOne) {
            selected.insert(maxView);
            available[maxView] = false;
        }
    }
}

float
GlobalViewSelection::benefitFromView(std::size_t i)
{
    SingleView::Ptr refV = views[settings.refViewNr];
    SingleView::Ptr tmpV = views[i];

    std::vector<std::size_t> nFeatIDs = tmpV->getFeatureIndices();

    // Go over all features visible in view i and reference view
    float benefit = 0;
    for (std::size_t k = 0; k < nFeatIDs.size(); ++k) {
        float score = 1.f;
        // Parallax with reference view
        math::Vec3f ftPos(features[nFeatIDs[k]].pos);
        float plx = parallax(ftPos, refV, tmpV);
        if (plx < settings.minParallax)
            score *= sqr(plx / 10.f);
        // Resolution compared to reference view
        float mfp = refV->footPrintScaled(ftPos);
        float nfp = tmpV->footPrint(ftPos);
        float ratio = mfp / nfp;
        if (ratio > 2.)
            ratio = 2. / ratio;
        else if (ratio > 1.)
            ratio = 1.;
        score *= ratio;
        // Parallax with other selected views that see the same feature
        IndexSet::const_iterator citV;
        for (citV = selected.begin(); citV != selected.end(); ++citV) {
            plx = parallax(ftPos, views[*citV], tmpV);
            if (plx < settings.minParallax)
                score *= sqr(plx / 10.f);
        }
        benefit += score;
    }
    return benefit;
}


MVS_NAMESPACE_END
