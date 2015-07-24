/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef UMVE_ADDIN_REPHOTOGRAPHER_HEADER
#define UMVE_ADDIN_REPHOTOGRAPHER_HEADER

#include <QWidget>
#include <QLineEdit>
#include <QFormLayout>

#include "mve/view.h"
#include "ogl/camera.h"

#include "scene_addins/addin_base.h"

class AddinRephotographer : public AddinBase
{
    Q_OBJECT

public:
    AddinRephotographer (void);
    QWidget* get_sidebar_widget (void);
    void set_scene_camera (ogl::Camera* camera);

protected:
    ogl::Camera* camera;
    QFormLayout* rephoto_form;
    QLineEdit* rephoto_source;
    QLineEdit* rephoto_color_dest;
    QLineEdit* rephoto_depth_dest;

protected slots:
    void on_rephoto (void);
    void on_rephoto_view (mve::View::Ptr view);
    void on_rephoto_all (void);
};

#endif /* UMVE_ADDIN_REPHOTOGRAPHER_HEADER */
