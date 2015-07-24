/*
 * Copyright (C) 2015, Ronny Klowsky, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef DMRECON_PATCH_SAMPLER_H
#define DMRECON_PATCH_SAMPLER_H

#include <map>
#include <memory>

#include "math/vector.h"
#include "dmrecon/defines.h"
#include "dmrecon/settings.h"
#include "dmrecon/single_view.h"

MVS_NAMESPACE_BEGIN

class PatchSampler
{
public:
    typedef std::shared_ptr<PatchSampler> Ptr;

public:
    /** Default constructor */
    PatchSampler();

    /** Constructor */
    PatchSampler(
        std::vector<SingleView::Ptr> const& _views,
        Settings const& _settings,
        int _x,          // pixel position
        int _y,
        float _depth,
        float _dzI,
        float _dzJ);

    /** Smart pointer PatchSampler constructor. */
    static PatchSampler::Ptr create(std::vector<SingleView::Ptr> const& views,
        Settings const& settings, int x, int _y,
        float _depth, float _dzI, float _dzJ);

    /** Draw color samples and derivatives in neighbor view v */
    void fastColAndDeriv(std::size_t v, Samples & color,
        Samples& deriv);

    /** Compute NCC between reference view and a neighbor view */
    float getFastNCC(std::size_t v);

    /**  */
    Samples const& getMasterColorSamples() const;

    /**  */
    float getMasterMeanColor() const;

    /**  */
    math::Vec3f const & getMidWorldPoint() const;

    /** Compute NCC between two neighboring views */
    float getNCC(std::size_t u, std::size_t v);

    /** Computes the sum of absolute differences between reference
        view and neighbor v with respect to color scale cs */
    float getSAD(std::size_t v, math::Vec3f const& cs);

    /** Computes the sum of squared differences between reference
        view and neighbor v with respect to color scale cs */
    float getSSD(std::size_t v, math::Vec3f const& cs);

    /**  */
    Samples const& getNeighColorSamples(std::size_t v);

    /**  */
    std::size_t getNrSamples() const;

    /**  */
    math::Vec3f getPatchNormal() const;

    /**  */
    bool succeeded(std::size_t v) const;

    /**  */
    void update(float newDepth, float newDzI, float newDzJ);

    /**  */
    float varInMasterPatch();


private:
    std::vector<SingleView::Ptr> const& views;
    Settings const& settings;

    /** precomputed mean and variance for NCC */
    math::Vec3f meanX;
    float sqrDevX;

    /** patch pos in image */
    math::Vec2i midPix;
    math::Vec2i topLeft;
    math::Vec2i bottomRight;

    /** mean patch color in master image before normalization */
    float masterMeanCol;

    /** filter width = 2 * offset + 1 */
    std::size_t offset;

    size_t nrSamples;

    /** depth and encoded normal */
    float depth;
    float dzI, dzJ;

    /** viewing rays according to patch in master view */
    std::vector<math::Vec3f> masterViewDirs;

    /** 3d position of patch points */
    Samples patchPoints;

    /** pixel colors of patch in master image */
    Samples masterColorSamples;

    /** samples in neighbor images */
    std::map<std::size_t, Samples> neighColorSamples;
    std::map<std::size_t, Samples> neighDerivSamples;
    std::map<std::size_t, PixelCoords> neighPosSamples;

    std::map<std::size_t, float> stepSize;

    void computePatchPoints();
    void computeMasterSamples();
    void computeNeighColorSamples(std::size_t v);

public:
    std::vector<bool> success;
};

inline PatchSampler::Ptr
PatchSampler::create(std::vector<SingleView::Ptr> const& views, Settings const& settings,
    int x, int y, float depth, float dzI, float dzJ)
{
    return PatchSampler::Ptr(new PatchSampler
        (views, settings, x, y, depth, dzI, dzJ));
}

inline Samples const&
PatchSampler::getMasterColorSamples() const
{
    return masterColorSamples;
}

inline Samples const&
PatchSampler::getNeighColorSamples(std::size_t v)
{
    if (neighColorSamples[v].empty())
        computeNeighColorSamples(v);
    return neighColorSamples[v];
}

inline float
PatchSampler::getMasterMeanColor() const
{
    return masterMeanCol;
}

inline math::Vec3f const&
PatchSampler::getMidWorldPoint() const
{
    return patchPoints[nrSamples/2];
}

inline std::size_t
PatchSampler::getNrSamples() const
{
    return nrSamples;
}

inline float
PatchSampler::varInMasterPatch()
{
    return sqrDevX / (3.f * (float) nrSamples);
}

MVS_NAMESPACE_END

#endif
