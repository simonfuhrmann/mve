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
