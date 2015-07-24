/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef UMVE_ADDIN_SELECTION
#define UMVE_ADDIN_SELECTION

#include <QCheckBox>

#include "ogl/vertex_array.h"

#include "scene_addins/addin_base.h"

class AddinSelection : public AddinBase
{
public:
    AddinSelection (void);
    virtual bool mouse_event (ogl::MouseEvent const& event);
    virtual void redraw_gui();

    void set_scene_camera (ogl::Camera* camera);

private:
    bool selection_active;

    int rect_start_x;
    int rect_start_y;
    int rect_current_x;
    int rect_current_y;

    ogl::Camera *camera;

    void show_selection_info (float left, float right, float top, float bottom);
};

#endif /* UMVE_ADDIN_SELECTION */
