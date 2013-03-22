#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdexcept>
#include <string>

#include "dmrecon/defines.h"

MVS_NAMESPACE_BEGIN

struct Settings
{
    Settings();
    ~Settings() {}

    float minNCC;
    float minParallax;
    unsigned int filterWidth;         // patch size is filterWidth*filterWidth
    float acceptNCC;
    float minRefineDiff;
    unsigned int maxIterations;
    unsigned int nrReconNeighbors;
    float scale;
    bool useColorScale;
    bool writePlyFile;
    std::string imageEmbedding;
    std::string plyPath;
    std::size_t refViewNr;
    unsigned int globalVSMax;
    std::string logPath;
};

MVS_NAMESPACE_END

#endif
