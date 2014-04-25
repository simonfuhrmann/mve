#ifndef DMRECON_LOCAL_VIEW_SELECTION_H
#define DMRECON_LOCAL_VIEW_SELECTION_H

#include "util/ref_ptr.h"
#include "dmrecon/view_selection.h"
#include "dmrecon/patch_sampler.h"
#include "dmrecon/single_view.h"

MVS_NAMESPACE_BEGIN

class LocalViewSelection : public ViewSelection
{
public:
    LocalViewSelection(
        std::vector<SingleView::Ptr> const& views,
        Settings const& settings,
        IndexSet const& globalViews,
        IndexSet const& propagated,
        PatchSampler::Ptr sampler);
    void performVS();
    void replaceViews(IndexSet const& toBeReplaced);

    bool success;

private:
    std::vector<SingleView::Ptr> const& views;
    PatchSampler::Ptr sampler;
};

MVS_NAMESPACE_END

#endif
