#ifndef GLOBALVIEWSELECTION_H
#define GLOBALVIEWSELECTION_H

#include <map>

#include "SingleView.h"
#include "ViewSelection.h"
#include "mve/bundlefile.h"


MVS_NAMESPACE_BEGIN


class GlobalViewSelection : public ViewSelection
{
public:
    GlobalViewSelection(SingleViewPtrList const& views,
        mve::BundleFile::FeaturePoints const& features,
        Settings const& settings);
    void performVS();

private:
    float benefitFromView(std::size_t i);

    SingleViewPtrList const& views;
    mve::BundleFile::FeaturePoints const& features;
};



MVS_NAMESPACE_END

#endif
