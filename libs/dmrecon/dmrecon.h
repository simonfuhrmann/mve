#ifndef DMRECON_DMRECON_H
#define DMRECON_DMRECON_H

#include <fstream>
#include <string>
#include <vector>
#include <queue>

#include "mve/bundle_file.h"
#include "mve/image.h"
#include "mve/scene.h"
#include "dmrecon/defines.h"
#include "dmrecon/patch_optimization.h"
#include "dmrecon/single_view.h"
#include "dmrecon/progress.h"

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

    std::size_t getRefViewNr() const;

private:
    mve::Scene::Ptr scene;
    mve::BundleFile::ConstPtr bundle;
    std::vector<SingleView::Ptr> views;

    Settings settings;
    std::priority_queue<QueueData> prQueue;
    IndexSet neighViews;
    std::vector<SingleView::Ptr> imgNeighbors;
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

inline std::size_t
DMRecon::getRefViewNr() const
{
    return settings.refViewNr;
}

MVS_NAMESPACE_END

#endif
