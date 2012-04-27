#ifndef VIEWSELECTION_H
#define VIEWSELECTION_H

#include "defines.h"
#include "Settings.h"
#include "util/refptr.h"


MVS_NAMESPACE_BEGIN

class ViewSelection;
typedef util::RefPtr<ViewSelection> ViewSelectionPtr;

class ViewSelection
{
public:
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
