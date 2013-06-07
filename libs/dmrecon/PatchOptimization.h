#ifndef DMRECON_PATCHOPTIMIZATION_H
#define DMRECON_PATCHOPTIMIZATION_H

#include <iostream>

#include "math/vector.h"
#include "util/refptr.h"
#include "dmrecon/defines.h"
#include "dmrecon/PatchSampler.h"
#include "dmrecon/SingleView.h"
#include "dmrecon/LocalViewSelection.h"

MVS_NAMESPACE_BEGIN

struct Status
{
    std::size_t iterationCount;
    bool converged;
    bool optiSuccess;
};

class PatchOptimization
{
public:
    PatchOptimization(
        std::vector<SingleView::Ptr> const& _views,
        Settings const& _settings,
        int _x,          // Pixel position
        int _y,
        float _depth,
        float _dzI,
        float _dzJ,
        IndexSet const& _globalViewIDs,
        IndexSet const& _localViewIDs);

    void computeColorScale();
    float computeConfidence();
    float derivNorm();
    void doAutoOptimization();
    float getDepth() const;
    float getDzI() const;
    float getDzJ() const;
    IndexSet const& getLocalViewIDs() const;
    math::Vec3f getNormal() const;
    float objFunValue();
    void optimizeDepthOnly();
    void optimizeDepthAndNormal();

private:
    std::vector<SingleView::Ptr> const& views;
    Settings const& settings;
    // initial values and settings
    const int midx;
    const int midy;

    float depth;
    float dzI, dzJ;                 // represents patch normal
    std::map<std::size_t, math::Vec3f, std::less<std::size_t> > colorScale;
    Status status;

    PatchSampler::Ptr sampler;
    std::vector<int> ii, jj;
    std::vector<float> pixel_weight;
    LocalViewSelection localVS;
};

inline float
PatchOptimization::getDepth() const
{
    return depth;
}

inline float
PatchOptimization::getDzI() const
{
    return dzI;
}

inline float
PatchOptimization::getDzJ() const
{
    return dzJ;
}

inline IndexSet const&
PatchOptimization::getLocalViewIDs() const
{
    return localVS.getSelectedIDs();
}

inline math::Vec3f
PatchOptimization::getNormal() const
{
    return sampler->getPatchNormal();
}

MVS_NAMESPACE_END

#endif
