/*
 * Copyright (C) 2015, Ronny Klowsky, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include "math/defines.h"
#include "math/matrix.h"
#include "math/vector.h"
#include "dmrecon/defines.h"
#include "dmrecon/mvs_tools.h"
#include "dmrecon/patch_sampler.h"

MVS_NAMESPACE_BEGIN

PatchSampler::PatchSampler(std::vector<SingleView::Ptr> const& _views,
    Settings const& _settings,
    int _x, int _y, float _depth, float _dzI, float _dzJ)
    : views(_views)
    , settings(_settings)
    , midPix(_x,_y)
    , masterMeanCol(0.f)
    , depth(_depth)
    , dzI(_dzI)
    , dzJ(_dzJ)
    , success(views.size(), false)
{
    SingleView::Ptr refV(views[settings.refViewNr]);
    mve::ByteImage::ConstPtr masterImg(refV->getScaledImg());

    offset = settings.filterWidth / 2;
    nrSamples = sqr(settings.filterWidth);

    /* initialize arrays */
    patchPoints.resize(nrSamples);
    masterColorSamples.resize(nrSamples);
    masterViewDirs.resize(nrSamples);

    /* compute patch position and check if it's valid */
    math::Vec2i h;
    h[0] = h[1] = offset;
    topLeft = midPix - h;
    bottomRight = midPix + h;
    if (topLeft[0] < 0 || topLeft[1] < 0
        || bottomRight[0] > masterImg->width()-1
        || bottomRight[1] > masterImg->height()-1)
        return;

    /* initialize viewing rays from master view */
    std::size_t count = 0;
    for (int j = topLeft[1]; j <= bottomRight[1]; ++j)
        for (int i = topLeft[0]; i <= bottomRight[0]; ++i)
            masterViewDirs[count++] = refV->viewRayScaled(i, j);

    /* initialize master color samples and 3d patch points */
    success[settings.refViewNr] = true;
    computeMasterSamples();
    computePatchPoints();
}

void
PatchSampler::fastColAndDeriv(std::size_t v, Samples& color, Samples& deriv)
{
    success[v] = false;
    SingleView::Ptr refV = views[settings.refViewNr];

    PixelCoords& imgPos = neighPosSamples[v];
    imgPos.resize(nrSamples);

    math::Vec3f const& p0 = patchPoints[nrSamples/2];
    /* compute pixel prints and decide on which MipMap-Level to draw
       the samples */
    float mfp = refV->footPrintScaled(p0);
    float nfp = views[v]->footPrint(p0);
    if (mfp <= 0.f) {
        std::cerr << "Error in getFastColAndDerivSamples! "
                  << "footprint in master view: " << mfp << std::endl;
        throw std::out_of_range("Negative pixel footprint");
    }
    if (nfp <= 0.f)
        return;
    float ratio = nfp / mfp;
    int mmLevel = 0;
    while (ratio < 0.5f) {
        ++mmLevel;
        ratio *= 2.f;
    }
    mmLevel = views[v]->clampLevel(mmLevel);

    /* compute step size for derivative */
    math::Vec3f p1(p0 + masterViewDirs[nrSamples/2]);
    float d = (views[v]->worldToScreen(p1, mmLevel)
        - views[v]->worldToScreen(patchPoints[12], mmLevel)).norm();
    if (!(d > 0.f)) {
        return;
    }
    stepSize[v] = 1.f / d;

    /* request according undistorted color image */
    mve::ByteImage::ConstPtr img(views[v]->getPyramidImg(mmLevel));
    int const w = img->width();
    int const h = img->height();

    /* compute image position and gradient direction for each sample
       point in neighbor image v */
    std::vector<math::Vec2f> gradDir(nrSamples);
    for (std::size_t i = 0; i < nrSamples; ++i)
    {
        math::Vec3f p0(patchPoints[i]);
        math::Vec3f p1(patchPoints[i] + masterViewDirs[i] * stepSize[v]);
        imgPos[i] = views[v]->worldToScreen(p0, mmLevel);
        // imgPos should be away from image border
        if (!(imgPos[i][0] > 0 && imgPos[i][0] < w-1 &&
                imgPos[i][1] > 0 && imgPos[i][1] < h-1)) {
            return;
        }
        gradDir[i] = views[v]->worldToScreen(p1, mmLevel) - imgPos[i];
    }

    /* draw the samples in the image */
    color.resize(nrSamples, math::Vec3f(0.f));
    deriv.resize(nrSamples, math::Vec3f(0.f));
    colAndExactDeriv(*img, imgPos, gradDir, color, deriv);

    /* normalize the gradient */
    for (std::size_t i = 0; i < nrSamples; ++i)
        deriv[i] /= stepSize[v];

    success[v] = true;
}

float
PatchSampler::getFastNCC(std::size_t v)
{
    if (neighColorSamples[v].empty())
        computeNeighColorSamples(v);
    if (!success[v])
        return -1.f;
    assert(success[settings.refViewNr]);
    math::Vec3f meanY(0.f);
    for (std::size_t i = 0; i < nrSamples; ++i)
        meanY += neighColorSamples[v][i];
    meanY /= (float) nrSamples;

    float sqrDevY = 0.f;
    float devXY = 0.f;
    for (std::size_t i = 0; i < nrSamples; ++i)
    {
        sqrDevY += (neighColorSamples[v][i] - meanY).square_norm();
        // Note: master color samples are normalized!
        devXY += (masterColorSamples[i] - meanX)
            .dot(neighColorSamples[v][i] - meanY);
    }
    float tmp = sqrt(sqrDevX * sqrDevY);
    assert(!MATH_ISNAN(tmp) && !MATH_ISNAN(devXY));
    if (tmp > 0)
        return (devXY / tmp);
    else
        return -1.f;
}

float
PatchSampler::getNCC(std::size_t u, std::size_t v)
{
    if (neighColorSamples[u].empty())
        computeNeighColorSamples(u);
    if (neighColorSamples[v].empty())
        computeNeighColorSamples(v);
    if (!success[u] || !success[v])
            return -1.f;

    math::Vec3f meanX(0.f);
    math::Vec3f meanY(0.f);
    for (std::size_t i = 0; i < nrSamples; ++i)
    {
        meanX += neighColorSamples[u][i];
        meanY += neighColorSamples[v][i];
    }
    meanX /= nrSamples;
    meanY /= nrSamples;

    float sqrDevX = 0.f;
    float sqrDevY = 0.f;
    float devXY = 0.f;
    for (std::size_t i = 0; i < nrSamples; ++i)
    {
        sqrDevX += (neighColorSamples[u][i] - meanX).square_norm();
        sqrDevY += (neighColorSamples[v][i] - meanY).square_norm();
        devXY += (neighColorSamples[u][i] - meanX)
            .dot(neighColorSamples[v][i] - meanY);
    }

    float tmp = sqrt(sqrDevX * sqrDevY);
    if (tmp > 0)
        return (devXY / tmp);
    else
        return -1.f;
}

float
PatchSampler::getSAD(std::size_t v, math::Vec3f const& cs)
{
    if (neighColorSamples[v].empty())
        computeNeighColorSamples(v);
    if (!success[v])
        return -1.f;

    float sum = 0.f;
    for (std::size_t i = 0; i < nrSamples; ++i) {
        for (int c = 0; c < 3; ++c) {
            sum += std::abs(cs[c] * neighColorSamples[v][i][c] -
                masterColorSamples[i][c]);
        }
    }
    return sum;
}

float
PatchSampler::getSSD(std::size_t v, math::Vec3f const& cs)
{
    if (neighColorSamples[v].empty())
        computeNeighColorSamples(v);
    if (!success[v])
        return -1.f;

    float sum = 0.f;
    for (std::size_t i = 0; i < nrSamples; ++i)
    {
        for (int c = 0; c < 3; ++c)
        {
            float diff = cs[c] * neighColorSamples[v][i][c] -
                masterColorSamples[i][c];
            sum += diff * diff;
        }
    }
    return sum;
}

math::Vec3f
PatchSampler::getPatchNormal() const
{
    std::size_t right = nrSamples/2 + offset;
    std::size_t left = nrSamples/2 - offset;
    std::size_t top = offset;
    std::size_t bottom = nrSamples - 1 - offset;

    math::Vec3f a(patchPoints[right] - patchPoints[left]);
    math::Vec3f b(patchPoints[top] - patchPoints[bottom]);
    math::Vec3f normal(a.cross(b));
    normal.normalize();

    return normal;
}

void
PatchSampler::update(float newDepth, float newDzI, float newDzJ)
{
    success.clear();
    success.resize(views.size(), false);
    depth = newDepth;
    dzI = newDzI;
    dzJ = newDzJ;
    success[settings.refViewNr] = true;
    computePatchPoints();
    neighColorSamples.clear();
    neighDerivSamples.clear();
    neighPosSamples.clear();
}

void
PatchSampler::computePatchPoints()
{
    SingleView::Ptr refV = views[settings.refViewNr];

    unsigned int count = 0;
    for (int j = topLeft[1]; j <= bottomRight[1]; ++j)
    {
        for (int i = topLeft[0]; i <= bottomRight[0]; ++i)
        {
            float tmpDepth = depth + (i - midPix[0]) * dzI +
                (j - midPix[1]) * dzJ;
            if (tmpDepth <= 0.f)
            {
                success[settings.refViewNr] = false;
                return;
            }
            patchPoints[count] = refV->camPos + tmpDepth *
                masterViewDirs[count];
            ++count;
        }
    }
}

void
PatchSampler::computeMasterSamples()
{
    SingleView::Ptr refV = views[settings.refViewNr];
    mve::ByteImage::ConstPtr img(refV->getScaledImg());

    /* draw color samples from image and compute mean color */
    std::size_t count = 0;
    std::vector<math::Vec2i> imgPos(nrSamples);
    for (int j = topLeft[1]; j <= bottomRight[1]; ++j)
        for (int i = topLeft[0]; i <= bottomRight[0]; ++i)
        {
            imgPos[count][0] = i;
            imgPos[count][1] = j;
            ++count;
        }
    getXYZColorAtPix(*img, imgPos, &masterColorSamples);

    masterMeanCol = 0.f;
    for (std::size_t i = 0; i < nrSamples; ++i)
        for (int c = 0; c < 3; ++c)
        {
            assert(masterColorSamples[i][c] >= 0 &&
                masterColorSamples[i][c] <= 1);
            masterMeanCol += masterColorSamples[i][c];
        }

    masterMeanCol /= 3.f * nrSamples;
    if (masterMeanCol < 0.01f || masterMeanCol > 0.99f) {
        success[settings.refViewNr] = false;
        return;
    }

    meanX.fill(0.f);

    /* normalize master samples so that average intensity over all
       channels is 1 and compute mean color afterwards */
    for (std::size_t i = 0; i < nrSamples; ++i)
    {
        masterColorSamples[i] /= masterMeanCol;
        meanX += masterColorSamples[i];
    }
    meanX /= nrSamples;
    sqrDevX = 0.f;

    /* compute variance (independent from actual mean) */
    for (std::size_t i = 0; i < nrSamples; ++i)
        sqrDevX += (masterColorSamples[i] - meanX).square_norm();
}

void
PatchSampler::computeNeighColorSamples(std::size_t v)
{
    SingleView::Ptr refV = views[settings.refViewNr];

    Samples & color = neighColorSamples[v];
    PixelCoords & imgPos = neighPosSamples[v];
    success[v] = false;

    /* compute pixel prints and decide on which MipMap-Level to draw
       the samples */
    math::Vec3f const & p0 = patchPoints[nrSamples/2];
    float mfp = refV->footPrintScaled(p0);
    float nfp = views[v]->footPrint(p0);
    if (mfp <= 0.f) {
        std::cerr << "Error in computeNeighColorSamples! "
                  << "footprint in master view: " << mfp << std::endl;
        throw std::out_of_range("Negative pixel print");
    }
    if (nfp <= 0.f)
        return;
    float ratio = nfp / mfp;

    int mmLevel = 0;
    while (ratio < 0.5f) {
        ++mmLevel;
        ratio *= 2.f;
    }
    mmLevel = views[v]->clampLevel(mmLevel);
    mve::ByteImage::ConstPtr img(views[v]->getPyramidImg(mmLevel));
    int const w = img->width();
    int const h = img->height();

    color.resize(nrSamples);
    imgPos.resize(nrSamples);

    for (std::size_t i = 0; i < nrSamples; ++i) {
        imgPos[i] = views[v]->worldToScreen(patchPoints[i], mmLevel);
        // imgPos should be away from image border
        if (!(imgPos[i][0] > 0 && imgPos[i][0] < w-1 &&
                imgPos[i][1] > 0 && imgPos[i][1] < h-1)) {
            return;
        }
    }
    getXYZColorAtPos(*img, imgPos, &color);
    success[v] = true;
}


MVS_NAMESPACE_END
