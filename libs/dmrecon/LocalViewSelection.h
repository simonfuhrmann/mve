#ifndef LOCALVIEWSELECTION_H
#define LOCALVIEWSELECTION_H

#include "util/refptr.h"
#include "dmrecon/ViewSelection.h"
#include "dmrecon/PatchSampler.h"
#include "dmrecon/SingleView.h"

MVS_NAMESPACE_BEGIN


class LocalViewSelection : public ViewSelection
{
public:
    LocalViewSelection(
        SingleViewPtrList const& views,
        Settings const& settings,
        IndexSet const& globalViews,
        IndexSet const& propagated,
        PatchSampler::Ptr sampler);
    void performVS();
    void replaceViews(IndexSet const& toBeReplaced);

    bool success;

private:
    SingleViewPtrList const& views;
    PatchSampler::Ptr sampler;
};


MVS_NAMESPACE_END

#endif
