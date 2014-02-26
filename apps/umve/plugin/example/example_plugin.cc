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

Q_EXPORT_PLUGIN2(ExamplePlugin, ExamplePlugin)
