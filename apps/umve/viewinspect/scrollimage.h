#ifndef SCROLL_IMAGE_HEADER
#define SCROLL_IMAGE_HEADER

#include <stdexcept>
#include <QScrollArea>

#include "clickimage.h"

#define ZOOM_IN_FACTOR (3.0 / 2.0)
#define ZOOM_OUT_FACTOR (2.0 / 3.0)

class ScrollImage : public QScrollArea
{
private:
    ClickImage* image;
    double scale_factor;
    bool scale_contents;

protected:
    void resizeEvent (QResizeEvent* event);
    void update_image_size (void);
    void max_image_size (void);
    void adjust_scrollbar (QScrollBar* bar, float factor);

public:
    ScrollImage (void);

    void set_pixmap (QPixmap const& pixmap);
    QPixmap const* get_pixmap (void) const;
    ClickImage* get_image (void);

    void set_auto_scale (bool value);
    void set_scale (double factor);
    void reset_scale (void);
    void zoom_in (void);
    void zoom_out (void);
    double get_scale (void) const;

    void save_image (std::string const& filename);
};

/* ---------------------------------------------------------------- */

inline void
ScrollImage::set_pixmap (QPixmap const& pixmap)
{
    this->image->setPixmap(pixmap);
    this->update_image_size();
}

inline void
ScrollImage::set_auto_scale (bool value)
{
    this->scale_contents = value;
    this->update_image_size();
}

inline void
ScrollImage::reset_scale (void)
{
    this->image->set_scale_factor(1.0);
    //this->scale_factor = 1.0;
    //this->image_label->adjustSize();
}

inline void
ScrollImage::zoom_out (void)
{
    this->set_scale(this->get_scale() * ZOOM_OUT_FACTOR);
}

inline void
ScrollImage::zoom_in (void)
{
    this->set_scale(this->get_scale() * ZOOM_IN_FACTOR);
}

inline double
ScrollImage::get_scale (void) const
{
    return this->image->get_scale_factor();
}

inline QPixmap const*
ScrollImage::get_pixmap (void) const
{
    return this->image->pixmap();
}

inline ClickImage*
ScrollImage::get_image (void)
{
    return this->image;
}

#endif /* SCROLL_IMAGE_HEADER */
