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
    ExamplePlugin (QWidget* parent = NULL);
    virtual ~ExamplePlugin (void);
    virtual QString get_title (void);
};

#endif /* HELLO_WORLD_TAB_HEADER */
