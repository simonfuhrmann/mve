#ifndef DMRECON_VIEWSELECTION_H
#define DMRECON_VIEWSELECTION_H

#include "util/refptr.h"
#include "dmrecon/defines.h"
#include "dmrecon/Settings.h"

MVS_NAMESPACE_BEGIN

class ViewSelection
{
public:
    typedef util::RefPtr<ViewSelection> Ptr;

    ViewSelection();
    ViewSelection(Settings const& settings);

public:
    IndexSet const& getSelectedIDs() const;

protected:
    Settings const& settings;
    std::vector<bool> available;
    IndexSet selected;
};


inline
ViewSelection::ViewSelection(Settings const& settings)
    :
    settings(settings)
{
}

inline IndexSet const&
ViewSelection::getSelectedIDs() const
{
    return selected;
}

MVS_NAMESPACE_END

#endif
