/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef UMVE_ADDIN_AXIS_RENDERER_HEADER
#define UMVE_ADDIN_AXIS_RENDERER_HEADER

#include <QCheckBox>

#include "ogl/vertex_array.h"

#include "scene_addins/addin_base.h"

class AddinAxisRenderer : public AddinBase
{
public:
    AddinAxisRenderer (void);
    QWidget* get_sidebar_widget (void);

protected:
    void paint_impl (void);

private:
    QCheckBox* render_cb;
    ogl::VertexArray::Ptr axis_renderer;
};

#endif /* UMVE_ADDIN_AXIS_RENDERER_HEADER */
