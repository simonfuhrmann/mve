#ifndef CLICK_IMAGE_HEADER
#define CLICK_IMAGE_HEADER

#include <QtGui>

/* Scalable image that features click signals. */
class ClickImage : public QLabel
{
    Q_OBJECT

private:
    double scale_factor;

protected:
    void mousePressEvent (QMouseEvent* event);

signals:
    void mouse_clicked (int x, int y, QMouseEvent* event);

public:
    ClickImage (void);
    ~ClickImage (void);

    double get_scale_factor (void) const;
    void set_scale_factor (double factor);
    void update_size (void);
};

/* ---------------------------------------------------------------- */

inline
ClickImage::ClickImage (void)
{
    this->scale_factor = 1.0;
}

inline
ClickImage::~ClickImage (void)
{
}

inline double
ClickImage::get_scale_factor (void) const
{
    return this->scale_factor;
}

inline void
ClickImage::set_scale_factor (double factor)
{
    this->scale_factor = factor;
    if (this->scale_factor == 1.0)
        this->adjustSize();
    else
        this->resize(this->scale_factor * this->pixmap()->size());
}

inline void
ClickImage::update_size (void)
{
    this->set_scale_factor(this->scale_factor);
}

inline void
ClickImage::mousePressEvent (QMouseEvent* event)
{
    QPoint const& pnt(event->pos());
    int img_x = 0;
    int img_y = 0;
    
    if (this->hasScaledContents())
    {
        QSize ps(this->pixmap()->size());
        QSize ws(this->size());
    
        float scale_x = (float)ps.width() / (float)ws.width();
        float scale_y = (float)ps.height() / (float)ws.height();

        img_x = (int)((float)pnt.x() * scale_x);
        img_y = (int)((float)pnt.y() * scale_y);
    }
    else
    {
        img_x = pnt.x();
        img_y = pnt.y();
    }
    emit this->mouse_clicked(img_x, img_y, event);
}

#endif /* CLICK_IMAGE_HEADER */
