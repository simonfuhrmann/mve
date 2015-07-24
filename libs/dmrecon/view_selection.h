/*
 * Copyright (C) 2015, Ronny Klowsky, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef DMRECON_VIEW_SELECTION_H
#define DMRECON_VIEW_SELECTION_H

#include <memory>

#include "dmrecon/defines.h"
#include "dmrecon/settings.h"

MVS_NAMESPACE_BEGIN

class ViewSelection
{
public:
    typedef std::shared_ptr<ViewSelection> Ptr;

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
