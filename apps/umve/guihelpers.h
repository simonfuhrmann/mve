#ifndef GUI_HELPERS_HEADER
#define GUI_HELPERS_HEADER

#include <QComboBox>
#include <QDialog>
#include <QLayout>
#include <QPushButton>
#include "mve/image.h"

#include "selectedview.h"

QPixmap get_pixmap_from_image (mve::ByteImage::ConstPtr img);
QWidget* get_separator (void);
QWidget* get_expander (void);
QWidget* get_wrapper (QLayout* layout, int margin = 0);

void set_qt_style (char const* style_name);

/* ---------------------------------------------------------------- */

/**
 * Dialog that asks the user for depthmap, confidence map,
 * normal image and color image.
 */
class PlyExportDialog : public QDialog
{
public:
    SelectedView selected_view;

    std::string depthmap;
    std::string confidence;
    std::string colorimage;

    QComboBox depthmap_cb;
    QComboBox confidence_cb;
    QComboBox colorimage_cb;

public:
    PlyExportDialog (mve::View::Ptr view, QWidget* parent = NULL);
    void accept (void);
};

/* ---------------------------------------------------------------- */

/** Widget that displays a header and collapsible content. */
class QCollapsible : public QWidget
{
    Q_OBJECT

private:
    QPushButton* collapse_but;
    QWidget* content;
    QWidget* content_indent;
    QWidget* content_wrapper;

private slots:
    void on_toggle_collapse (void);

public:
    QCollapsible (QString title, QWidget* content);
    void set_collapsed (bool value);
    void set_collapsible (bool value);
    void set_content_indent (int pixels);
};

/* ---------------------------------------------------------------- */

/** File or directory selector button. */
class QFileSelector : public QPushButton
{
    Q_OBJECT

private:
    bool dironly;
    std::string filename;
    std::size_t ellipsize;

private slots:
    void on_clicked (void);

public:
    QFileSelector (QWidget* parent = NULL);
    void set_directory_mode (void);
    void set_ellipsize (std::size_t chars);
    std::string const& get_filename (void) const;
};

#endif /* GUI_HELPERS_HEADER */
