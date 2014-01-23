#ifndef MAIN_WINDOW_TAB_HEADER
#define MAIN_WINDOW_TAB_HEADER

#include <QWidget>

class MainWindowTab : public QWidget
{
    Q_OBJECT

protected:
    bool is_tab_active;

signals:
    void tab_activated (void);

public:
    MainWindowTab (QWidget* parent);
    virtual ~MainWindowTab (void);

    void set_tab_active (bool tab_active);
};

#endif // MAIN_WINDOW_TAB_HEADER
