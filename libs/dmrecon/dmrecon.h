/*
 * Copyright (C) 2015, Ronny Klowsky, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef DMRECON_DMRECON_H
#define DMRECON_DMRECON_H

#include <fstream>
#include <string>
#include <vector>
#include <queue>

#include "mve/bundle.h"
#include "mve/image.h"
#include "mve/scene.h"
#include "dmrecon/defines.h"
#include "dmrecon/patch_optimization.h"
#include "dmrecon/single_view.h"
#include "dmrecon/progress.h"

MVS_NAMESPACE_BEGIN

struct QueueData
{
    int x;
    int y;
    float confidence;
    float depth;
    float dz_i, dz_j;
    IndexSet localViewIDs;

    bool operator< (const QueueData& rhs) const;
};

class DMRecon
{
public:
    DMRecon(mve::Scene::Ptr scene, Settings const& settings);

    std::size_t getRefViewNr() const;
    Progress const& getProgress() const;
    Progress& getProgress();
    void start();

private:
    mve::Scene::Ptr scene;
    mve::Bundle::ConstPtr bundle;
    std::vector<SingleView::Ptr> views;

    Settings settings;
    std::priority_queue<QueueData> prQueue;
    IndexSet neighViews;
    std::vector<SingleView::Ptr> imgNeighbors;
    int width;
    int height;
    Progress progress;

    void analyzeFeatures();
    void globalViewSelection();
    void processFeatures();
    void processQueue();
    void refillQueueFromLowRes();
};

/* ------------------------- Implementation ----------------------- */

inline bool
QueueData::operator< (const QueueData& rhs) const
{
    return (confidence < rhs.confidence);
}

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
