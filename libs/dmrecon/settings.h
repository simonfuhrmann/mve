#ifndef DMRECON_SETTINGS_H
#define DMRECON_SETTINGS_H

#include <stdexcept>
#include <string>
#include <limits>

#include "math/vector.h"
#include "dmrecon/defines.h"

MVS_NAMESPACE_BEGIN

struct Settings
{
    Settings (void);

    /** The reference view ID to reconstruct. */
    std::size_t refViewNr;

    /** Input image emebdding. */
    std::string imageEmbedding;

    /** Size of the patch is width * width, defaults to 5x5. */
    unsigned int filterWidth;
    float minNCC;
    float minParallax;
    float acceptNCC;
    float minRefineDiff;
    unsigned int maxIterations;
    unsigned int nrReconNeighbors;
    unsigned int globalVSMax;
    int scale;
    bool useColorScale;
    bool writePlyFile;

    /** Features outside the AABB are ignored. */
    math::Vec3f aabbMin;
    math::Vec3f aabbMax;

    std::string plyPath;
    std::string logPath;

    bool keepDzMap;
    bool keepConfidenceMap;
    bool quiet;
};

/* ------------------------- Implementation ----------------------- */

inline
Settings::Settings (void)
    : refViewNr(0)
    , imageEmbedding("undistorted")
    , filterWidth(5)
    , minNCC(0.3f)
    , minParallax(10.f)
    , acceptNCC(0.6f)
    , minRefineDiff(0.001f)
    , maxIterations(20)
    , nrReconNeighbors(4)
    , globalVSMax(20)
    , scale(0)
    , useColorScale(true)
    , writePlyFile(false)
    , aabbMin(-std::numeric_limits<float>::max())
    , aabbMax(std::numeric_limits<float>::max())
    , keepDzMap(false)
    , keepConfidenceMap(false)
    , quiet(false)
{
}

MVS_NAMESPACE_END

#endif /* DMRECON_SETTINGS_H */
