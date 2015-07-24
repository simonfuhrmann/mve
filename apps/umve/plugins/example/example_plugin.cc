/*
 * Copyright (C) 2015, Benjamin Richter
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <QtCore>

#include "example_plugin.h"

ExamplePlugin::ExamplePlugin (QWidget* parent)
    : MainWindowTab(parent)
{
}

ExamplePlugin::~ExamplePlugin()
{
}

QString
ExamplePlugin::get_title (void)
{
    return tr("Hello World");
}

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(ExamplePlugin, ExamplePlugin)
#endif
