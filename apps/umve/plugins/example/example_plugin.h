/*
 * Copyright (C) 2015, Benjamin Richter
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef HELLO_WORLD_TAB_HEADER
#define HELLO_WORLD_TAB_HEADER

#include <QWidget>

#include "mainwindowtab.h"

class ExamplePlugin : public MainWindowTab
{
    Q_OBJECT
#if QT_VERSION >= 0x050000
    Q_PLUGIN_METADATA(IID MainWindowTab_iid)
#endif
    Q_INTERFACES(MainWindowTab)

public:
    ExamplePlugin (QWidget* parent = nullptr);
    virtual ~ExamplePlugin (void);
    virtual QString get_title (void);
};

#endif /* HELLO_WORLD_TAB_HEADER */
