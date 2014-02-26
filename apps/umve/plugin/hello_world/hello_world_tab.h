#ifndef HELLO_WORLD_TAB_HEADER
#define HELLO_WORLD_TAB_HEADER

#include <QWidget>

#include "mainwindowtab.h"

class HelloWorldTab : public MainWindowTab
{
    Q_OBJECT

public:
    HelloWorldTab (QWidget* parent = NULL);
    virtual ~HelloWorldTab (void);
    virtual QString get_title (void);
};

#endif /* HELLO_WORLD_TAB_HEADER */
