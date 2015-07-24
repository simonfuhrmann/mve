/*
 * Copyright (C) 2015, Ronny Klowsky, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef DMRECON_GLOBAL_VIEW_SELECTION_H
#define DMRECON_GLOBAL_VIEW_SELECTION_H

#include <map>

#include "mve/bundle.h"
#include "dmrecon/single_view.h"
#include "dmrecon/view_selection.h"

MVS_NAMESPACE_BEGIN

class GlobalViewSelection : public ViewSelection
{
public:
    GlobalViewSelection(std::vector<SingleView::Ptr> const& views,
        mve::Bundle::Features const& features,
        Settings const& settings);
    void performVS();

private:
    float benefitFromView(std::size_t i);

    std::vector<SingleView::Ptr> const& views;
    mve::Bundle::Features const& features;
};

MVS_NAMESPACE_END

#endif
