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

    /* title of this tab when added to a QTabWidget */
    virtual QString get_title (void) = 0;

    /* should be called when the parent QTabWidget switches to this tab */
    void set_tab_active (bool tab_active);
};

#define MainWindowTab_iid "de.tu-darmstadt.informatik.gris.UMVE.MainWindowTab"
Q_DECLARE_INTERFACE(MainWindowTab, MainWindowTab_iid)

#endif // MAIN_WINDOW_TAB_HEADER
