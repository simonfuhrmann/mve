#include "dmrecon/settings.h"

MVS_NAMESPACE_BEGIN

Settings::Settings()
    : minNCC(0.3f)
    , minParallax(10.f)
    , filterWidth(5)
    , acceptNCC(0.6f)
    , minRefineDiff(0.001f)
    , maxIterations(20)
    , nrReconNeighbors(4)
    , scale(0)
    , useColorScale(true)
    , writePlyFile(false)
    , imageEmbedding("undistorted")
    , refViewNr(0)
    , globalVSMax(20)
    , keepDzMap(false)
    , keepConfidenceMap(false)
    , quiet(false)
{
}

MVS_NAMESPACE_END
