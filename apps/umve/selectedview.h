#ifndef UMVE_SELECTEDVIEW_HEADER
#define UMVE_SELECTEDVIEW_HEADER

#include <QtGui>

#include "mve/view.h"

class SelectedView : public QFrame
{
private:
    mve::View::Ptr view;

    QLabel* viewname;
    QLabel* image;
    QLabel* viewinfo;
    //QComboBox* embeddings;

private:
    void reset_view (void);

public:
    SelectedView (void);

    void set_view (mve::View::Ptr view);
    mve::View::Ptr get_view (void) const;
    std::string selected_embedding (void) const;

    void fill_embeddings (QComboBox& cb, mve::ImageType type,
        std::string const& default_name = "") const;
};

/* ---------------------------------------------------------------- */

inline mve::View::Ptr
SelectedView::get_view (void) const
{
    return this->view;
}

#endif /* UMVE_SELECTEDVIEW_HEADER */
