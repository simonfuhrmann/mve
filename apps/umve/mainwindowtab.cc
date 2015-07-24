/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <iostream>

#include "mainwindowtab.h"

MainWindowTab::MainWindowTab (QWidget *parent)
    : QWidget(parent)
{
}

MainWindowTab::~MainWindowTab (void)
{
}

void
MainWindowTab::set_tab_active (bool active)
{
    this->is_tab_active = active;
    if (active)
        emit this->tab_activated();
}
