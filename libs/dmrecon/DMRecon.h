#ifndef DMRECON_H
#define DMRECON_H

#include <fstream>
#include <string>
#include <vector>
#include <queue>

#include "mve/bundlefile.h"
#include "mve/image.h"
#include "mve/scene.h"

#include "defines.h"
#include "PatchOptimization.h"
#include "SingleView.h"
#include "Progress.h"


MVS_NAMESPACE_BEGIN

struct QueueData
{
    int x;              // pixel position
    int y;
    float confidence;
    float depth;
    float dz_i, dz_j;
    IndexSet localViewIDs;

    bool operator< (const QueueData& rhs) const;
};

inline bool
QueueData::operator< (const QueueData& rhs) const
{
    return (confidence < rhs.confidence);
}

class DMRecon
{
public:
    DMRecon(mve::Scene::Ptr scene, Settings const& settings);
    ~DMRecon();

    Progress const& getProgress() const;
    Progress& getProgress();
    void start();            // according to settings

private:
    mve::Scene::Ptr scene;
    mve::BundleFile::ConstPtr bundle;
    SingleViewPtrList views;

    Settings settings;
    std::priority_queue<QueueData> prQueue;
    IndexSet neighViews;
    std::vector<SingleViewPtr> imgNeighbors;
    int width;
    int height;
    Progress progress;
    std::ofstream log;

    void analyzeFeatures();
    void globalViewSelection();
    void processFeatures();
    void processQueue();
    void refillQueueFromLowRes();
};


inline const Progress&
DMRecon::getProgress() const
{
    return progress;
}

inline Progress&
DMRecon::getProgress()
{
    return progress;
}

MVS_NAMESPACE_END

#endif
