/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef UMVE_ADDIN_DM_TRIANGULATE_HEADER
#define UMVE_ADDIN_DM_TRIANGULATE_HEADER

#include <QWidget>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QFormLayout>

#include "selectedview.h"

#include "scene_addins/addin_base.h"

class AddinDMTriangulate : public AddinBase
{
    Q_OBJECT

public:
    AddinDMTriangulate (void);
    void set_selected_view (SelectedView* view);
    QWidget* get_sidebar_widget (void);

private slots:
    void on_triangulate_clicked (void);
    void on_select_colorimage (QString name);
    void on_view_selected (void);

private:
    SelectedView* view;
    QFormLayout* dm_form;
    QComboBox* dm_depthmap_cb;
    QComboBox* dm_colorimage_cb;
    QDoubleSpinBox* dm_depth_disc;
    QPushButton* dm_triangulate_but;
};

#endif /* UMVE_ADDIN_DM_TRIANGULATE_HEADER */
