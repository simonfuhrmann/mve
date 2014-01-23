#ifndef UMVE_SELECTEDVIEW_HEADER
#define UMVE_SELECTEDVIEW_HEADER

#include <QComboBox>
#include <QLabel>
#include <QFrame>

#include "mve/view.h"

class SelectedView : public QFrame
{
    Q_OBJECT

private:
    mve::View::Ptr view;

    QLabel* viewname;
    QLabel* image;
    QLabel* viewinfo;

private:
    void reset_view (void);

public:
    SelectedView (void);

    void set_view (mve::View::Ptr view);
    mve::View::Ptr get_view (void) const;

    void fill_embeddings (QComboBox& cb, mve::ImageType type,
        std::string const& default_name = "") const;

signals:
    void view_selected (mve::View::Ptr view);
};

/* ---------------------------------------------------------------- */

inline mve::View::Ptr
SelectedView::get_view (void) const
{
    return this->view;
}

#endif /* UMVE_SELECTEDVIEW_HEADER */
