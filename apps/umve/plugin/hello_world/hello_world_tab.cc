#include <QtCore>

#include "hello_world_tab.h"

HelloWorldTab::HelloWorldTab (QWidget* parent)
    : MainWindowTab(parent)
{
}

HelloWorldTab::~HelloWorldTab()
{
}

QString
HelloWorldTab::get_title (void)
{
    return tr("Hello World");
}

Q_EXPORT_PLUGIN2(HelloWorldTab, HelloWorldTab)
