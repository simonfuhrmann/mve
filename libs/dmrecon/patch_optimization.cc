/*
 * Copyright (C) 2015, Ronny Klowsky, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include "util/string.h"
#include "math/algo.h"
#include "math/defines.h"
#include "math/matrix.h"
#include "math/matrix_tools.h"
#include "math/vector.h"
#include "dmrecon/patch_optimization.h"
#include "dmrecon/settings.h"

MVS_NAMESPACE_BEGIN

PatchOptimization::PatchOptimization(
    std::vector<SingleView::Ptr> const& _views,
    Settings const& _settings,
    int _x,          // Pixel position
    int _y,
    float _depth,
    float _dzI,
    float _dzJ,
    IndexSet const & _globalViewIDs,
    IndexSet const & _localViewIDs)
    :
    views(_views),
    settings(_settings),
    midx(_x),
    midy(_y),
    depth(_depth),
    dzI(_dzI),
    dzJ(_dzJ),
    sampler(PatchSampler::create(views, settings, midx, midy, depth, dzI, dzJ)),
    ii(sqr(settings.filterWidth)),
    jj(sqr(settings.filterWidth)),
    localVS(views, settings, _globalViewIDs, _localViewIDs, sampler)
{
    status.iterationCount = 0;
    status.optiSuccess = true;
    status.converged = false;

    if (!sampler->success[settings.refViewNr]) {
        // Sampler could not be initialized properly
        status.optiSuccess = false;
        return;
    }

    std::size_t count = 0;

    int halfFW = (int) settings.filterWidth / 2;
    pixel_weight.resize(sampler->getNrSamples());
    for (int j = -halfFW; j <= halfFW; ++j)
        for (int i = -halfFW; i <= halfFW; ++i) {
            ii[count] = i;
            jj[count] = j;
            pixel_weight[count] = 1.f;
            ++count;
        }

    localVS.performVS();
    if (!localVS.success) {
        status.optiSuccess = false;
        return;
    }

    // brute force initialize all colorScale entries
    float masterMeanCol = sampler->getMasterMeanColor();
    for (std::size_t idx = 0; idx < views.size(); ++idx) {
        colorScale[idx] = math::Vec3f(1.f / masterMeanCol);
    }
    computeColorScale();
}

void
PatchOptimization::computeColorScale()
{
    if (!settings.useColorScale)
        return;
    Samples const & mCol = sampler->getMasterColorSamples();
    IndexSet const & neighIDs = localVS.getSelectedIDs();
    IndexSet::const_iterator id;
    for (id = neighIDs.begin(); id != neighIDs.end(); ++id)
    {
        // just copied from old mvs:
        Samples const & nCol = sampler->getNeighColorSamples(*id);
        if (!sampler->success[*id])
            return;
        for (int c = 0; c < 3; ++c) {
            // for each color channel
            float ab = 0.f;
            float aa = 0.f;
            for (std::size_t i = 0; i < mCol.size(); ++i) {
                ab += (mCol[i][c] - nCol[i][c] * colorScale[*id][c]) * nCol[i][c];
                aa += sqr(nCol[i][c]);
            }
            if (std::abs(aa) > 1e-6) {
                colorScale[*id][c] += ab / aa;
                if (colorScale[*id][c] > 1e3)
                    status.optiSuccess = false;
            }
            else
                status.optiSuccess = false;
        }
    }
}

float
PatchOptimization::computeConfidence()
{
    SingleView::Ptr refV = views[settings.refViewNr];
    if (!status.converged)
        return 0.f;

    /* Compute mean NCC between reference view and local neighbors,
       where each NCC has to be higher than acceptance NCC */
    float meanNCC = 0.f;
    IndexSet const & neighIDs = localVS.getSelectedIDs();
    IndexSet::const_iterator id;
    for (id = neighIDs.begin(); id != neighIDs.end(); ++id) {
        meanNCC += sampler->getFastNCC(*id);
    }
    meanNCC /= neighIDs.size();

    float score = (meanNCC - settings.acceptNCC) /
                  (1.f - settings.acceptNCC);

    /* Compute angle between estimated surface normal and view direction
       and weight current score with dot product */
    math::Vec3f viewDir(refV->viewRayScaled(midx, midy));
    math::Vec3f normal(sampler->getPatchNormal());
    float dotP = - normal.dot(viewDir);
    if (dotP < 0.2f) {
        return 0.f;
    }
    return score;
}

float
PatchOptimization::derivNorm()
{
    IndexSet const & neighIDs = localVS.getSelectedIDs();
    IndexSet::const_iterator id;
    std::size_t nrSamples = sampler->getNrSamples();

    float norm(0);
    for (id = neighIDs.begin(); id != neighIDs.end(); ++id)
    {
        Samples nCol, nDeriv;
        sampler->fastColAndDeriv(*id, nCol, nDeriv);
        if (!sampler->success[*id]) {
            status.optiSuccess = false;
            return -1.f;
        }

        math::Vec3f cs(colorScale[*id]);
        for (std::size_t i = 0; i < nrSamples; ++i) {
            norm += pixel_weight[i] * (cs.cw_mult(nDeriv[i])).square_norm();
        }
    }
    return norm;
}

void
PatchOptimization::doAutoOptimization()
{
    if (!localVS.success || !status.optiSuccess) {
        return;
    }

    // first four iterations only refine depth
    while ((status.iterationCount < 4) && (status.optiSuccess)) {
        optimizeDepthOnly();
        ++status.iterationCount;
    }

    bool viewRemoved = false;
    bool converged = false;
    while (status.iterationCount < settings.maxIterations &&
        localVS.success && status.optiSuccess)
    {
        IndexSet const & neighIDs = localVS.getSelectedIDs();
        IndexSet::const_iterator id;

        std::vector<float> oldNCC;
        for (id = neighIDs.begin(); id != neighIDs.end(); ++id) {
            oldNCC.push_back(sampler->getFastNCC(*id));
        }

        status.optiSuccess = false;
        if (status.iterationCount % 5 == 4 || viewRemoved) {
            optimizeDepthAndNormal();
            computeColorScale();
            viewRemoved = false;
        }
        else {
            optimizeDepthOnly();
        }

        if (!status.optiSuccess)
            return;
        converged = true;
        std::size_t count = 0;
        IndexSet toBeReplaced;

        for (id = neighIDs.begin(); id != neighIDs.end(); ++id, ++count)
        {
            float ncc = sampler->getFastNCC(*id);
            if (std::abs(ncc - oldNCC[count]) > settings.minRefineDiff)
                converged = false;
            if ((ncc < settings.acceptNCC) ||
                (status.iterationCount == 14 &&
                 std::abs(ncc - oldNCC[count]) > settings.minRefineDiff))
            {
                toBeReplaced.insert(*id);
                viewRemoved = true;
            }
        }

        if (viewRemoved) {
            localVS.replaceViews(toBeReplaced);
            if (!localVS.success) {
                return;
            }
            computeColorScale();
        }
        else if (!status.optiSuccess) {
            // all views valid but optimization failed -> give up
            return;
        }
        else if (converged) {
            status.converged = true;
            return;
        }
        ++status.iterationCount;
    }
}

float
PatchOptimization::objFunValue()
{
    Samples const & mCol = sampler->getMasterColorSamples();
    std::size_t nrSamples = sampler->getNrSamples();
    IndexSet const & neighIDs = localVS.getSelectedIDs();
    IndexSet::const_iterator id;
    float obj = 0.f;
    for (id = neighIDs.begin(); id != neighIDs.end(); ++id) {
        Samples const & nCol = sampler->getNeighColorSamples(*id);
        if (!sampler->success[*id])
            return -1.f;
        math::Vec3f cs(colorScale[*id]);
        for (std::size_t i = 0; i < nrSamples; ++i) {
            obj += pixel_weight[i] * (mCol[i] - cs.cw_mult(nCol[i])).square_norm();
        }
    }
    return obj;
}

void
PatchOptimization::optimizeDepthOnly()
{
    float numerator = 0.f;
    float denom = 0.f;
    Samples const & mCol = sampler->getMasterColorSamples();
    IndexSet const & neighIDs = localVS.getSelectedIDs();
    IndexSet::const_iterator id;
    std::size_t nrSamples = sampler->getNrSamples();

    for (id = neighIDs.begin(); id != neighIDs.end(); ++id)
    {
        Samples nCol, nDeriv;
        sampler->fastColAndDeriv(*id, nCol, nDeriv);
        if (!sampler->success[*id]) {
            status.optiSuccess = false;
            return;
        }

        math::Vec3f cs(colorScale[*id]);
        for (std::size_t i = 0; i < nrSamples; ++i) {
            numerator += pixel_weight[i] * (cs.cw_mult(nDeriv[i])).dot
                (mCol[i] - cs.cw_mult(nCol[i]));
            denom += pixel_weight[i] * (cs.cw_mult(nDeriv[i])).square_norm();
        }
    }

    if (denom > 0) {
        depth += numerator / denom;
        sampler->update(depth, dzI, dzJ);
        if (sampler->success[settings.refViewNr])
            status.optiSuccess = true;
        else
            status.optiSuccess = false;
    }
}

void
PatchOptimization::optimizeDepthAndNormal()
{
    if (!localVS.success) {
        return;
    }
    IndexSet const & neighIDs = localVS.getSelectedIDs();
    std::size_t nrSamples = sampler->getNrSamples();

    // Solve linear system A*x = b using Moore-Penrose pseudoinverse
    // Fill matrix ATA and vector ATb:
    math::Matrix3d ATA(0.f);
    math::Vec3d ATb(0.f);
    Samples const & mCol = sampler->getMasterColorSamples();
    IndexSet::const_iterator id;
    std::size_t row = 0;
    for (id = neighIDs.begin(); id != neighIDs.end(); ++id)
    {
        Samples nCol, nDeriv;
        sampler->fastColAndDeriv(*id, nCol, nDeriv);
        if (!sampler->success[*id]) {
            status.optiSuccess = false;
            return;
        }
        math::Vec3f cs(colorScale[*id]);
        for (std::size_t i = 0; i < nrSamples; ++i) {
            for (int c = 0; c < 3; ++c) {
                math::Vec3f a_i;
                a_i[0] = pixel_weight[i] *         cs[c] * nDeriv[i][c];
                a_i[1] = pixel_weight[i] * ii[i] * cs[c] * nDeriv[i][c];
                a_i[2] = pixel_weight[i] * jj[i] * cs[c] * nDeriv[i][c];
                float b_i = pixel_weight[i] * (mCol[i][c] - cs[c] * nCol[i][c]);
                assert(!MATH_ISINF(a_i[0]));
                assert(!MATH_ISINF(a_i[1]));
                assert(!MATH_ISINF(a_i[2]));
                assert(!MATH_ISINF(b_i));
                ATA(0,0) += a_i[0] * a_i[0];
                ATA(0,1) += a_i[0] * a_i[1];
                ATA(0,2) += a_i[0] * a_i[2];
                ATA(1,1) += a_i[1] * a_i[1];
                ATA(1,2) += a_i[1] * a_i[2];
                ATA(2,2) += a_i[2] * a_i[2];
                ATb += a_i * b_i;
                ++row;
            }
        }
    }
    ATA(1,0) = ATA(0,1); ATA(2,0) = ATA(0,2); ATA(2,1) = ATA(1,2);
    double detATA = math::matrix_determinant(ATA);
    if (detATA == 0.f) {
        status.optiSuccess = false;
        return;
    }
    math::Matrix3d ATAinv = math::matrix_inverse(ATA, detATA);
    math::Vec3f X = ATAinv * ATb;

    // update depth and normal
    dzI += X[1];
    dzJ += X[2];
    depth += X[0];
    sampler->update(depth, dzI, dzJ);
    if (sampler->success[settings.refViewNr])
        status.optiSuccess = true;
    else
        status.optiSuccess = false;
}


MVS_NAMESPACE_END
