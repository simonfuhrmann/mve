#include <iostream>

#include "mve/imagebase.h"
#include "mve/image.h"

#include "imageinspector.h"

ImageInspectorWidget::ImageInspectorWidget (void)
{
    this->label_clickpos = new QLabel();
    this->label_values = new QLabel();
    this->label_clickpos->setAlignment(Qt::AlignRight);
    this->label_values->setAlignment(Qt::AlignRight);
    this->inspect_x = -1;
    this->inspect_y = -1;

    this->image_widget1 = new ClickImage();
    this->image_widget2 = new ClickImage();
    this->image_widget3 = new ClickImage();
    this->reset_images();

    this->connect(this->image_widget1,
        SIGNAL(mouse_clicked(int, int, QMouseEvent*)),
        this, SLOT(on_large_image_clicked(int, int, QMouseEvent*)));
    this->connect(this->image_widget2,
        SIGNAL(mouse_clicked(int, int, QMouseEvent*)),
        this, SLOT(on_medium_image_clicked(int, int, QMouseEvent*)));
    this->connect(this->image_widget3,
        SIGNAL(mouse_clicked(int, int, QMouseEvent*)),
        this, SLOT(on_small_image_clicked(int, int, QMouseEvent*)));

    QFormLayout* info_layout = new QFormLayout();
    info_layout->setVerticalSpacing(0);
    info_layout->addRow("Position:", this->label_clickpos);
    info_layout->addRow("Values:", this->label_values);

    QVBoxLayout* main_layout = new QVBoxLayout();
    main_layout->addLayout(info_layout);
    main_layout->addWidget(this->image_widget1);
    main_layout->addWidget(this->image_widget2);
    main_layout->addWidget(this->image_widget3);
    main_layout->addSpacerItem(new QSpacerItem(0, 0,
        QSizePolicy::Minimum, QSizePolicy::Expanding));

    this->setLayout(main_layout);
}

/* ---------------------------------------------------------------- */

void
ImageInspectorWidget::set_image (mve::ImageBase::ConstPtr image)
{
    this->image = image;

    int mag_x = this->inspect_x;
    int mag_y = this->inspect_y;
    this->reset_images();

    if (mag_x >= 0 && mag_y >= 0)
        this->magnify(mag_x, mag_y);
}

/* ---------------------------------------------------------------- */

void
ImageInspectorWidget::magnify (int x, int y)
{
    if (this->image.get() == 0)
        return;

    std::size_t iw = this->image->width();
    std::size_t ih = this->image->height();

    x = std::max(0, std::min((int)(iw - 1), x));
    y = std::max(0, std::min((int)(ih - 1), y));

    this->inspect_x = x;
    this->inspect_y = y;

    QImage image1 = this->get_magnified(x, y,
        MAGNIFY_LARGE_PATCH, MAGNIFY_LARGE_SCALE);
    QImage image2 = this->get_magnified(x, y,
        MAGNIFY_MEDIUM_PATCH, MAGNIFY_MEDIUM_SCALE);
    QImage image3 = this->get_magnified(x, y,
        MAGNIFY_SMALL_PATCH, MAGNIFY_SMALL_SCALE);

    this->update_value_label(x, y);

    this->image_widget1->setPixmap(QPixmap::fromImage(image1));
    this->image_widget2->setPixmap(QPixmap::fromImage(image2));
    this->image_widget3->setPixmap(QPixmap::fromImage(image3));
    this->label_clickpos->setText(tr("%1%2%3")
        .arg(tr("%1").arg(x), tr("x"), tr("%1").arg(y)));
}

/* ---------------------------------------------------------------- */

QImage
ImageInspectorWidget::get_magnified (int x, int y, int size, int scale)
{
    std::size_t iw = this->image->width();
    std::size_t ih = this->image->height();

    QImage img(size * scale, size * scale, QImage::Format_RGB32);
    img.fill(0xff00ff);
    //std::cout << "Image has size " << (size * scale)
    //    << " Pixel " << x << "x" << y << ", size " << size
    //    << ", scale " << scale << std::endl;

    int left = x - size / 2;
    int right = x + size / 2;
    int top = y - size / 2;
    int bottom = y + size / 2;

    int safe_left = std::max(0, left);
    int safe_right = std::min((int)(iw - 1), right);
    int safe_top = std::max(0, top);
    int safe_bottom = std::min((int)(ih - 1), bottom);

    for (int iy = safe_top; iy <= safe_bottom; ++iy)
        for (int ix = safe_left; ix <= safe_right; ++ix)
        {
            int rx = ix - left;
            int ry = iy - top;
            /*
            std::cout << "Reading pixel at " << rx << " " << ry
                << " to region (" << rx * scale << ", " << ry * scale
                << ") -> (" << (rx + 1) * scale << ", " << (ry + 1) * scale
                << ")" << std::endl;
            */
            unsigned int color = this->get_image_color(ix, iy);
            for (int iiy = ry * scale; iiy < (ry + 1) * scale; ++iiy)
                for (int iix = rx * scale; iix < (rx + 1) * scale; ++iix)
                    img.setPixel(iix, iiy, color);
        }

    return img;
}

/* ---------------------------------------------------------------- */

unsigned int
ImageInspectorWidget::get_image_color (int x, int y)
{
    std::size_t ic = this->image->channels();

    unsigned int color;
    switch (this->image->get_type())
    {
        case mve::IMAGE_TYPE_UINT8:
        {
            mve::ByteImage::ConstPtr img
                = (mve::ByteImage::ConstPtr)this->image;
            unsigned int red, green, blue;
            if (ic >= 3)
            {
                red = img->at(x, y, 0);
                green = img->at(x, y, 1);
                blue = img->at(x, y, 2);
            }
            else
            {
                red = green = blue = img->at(x, y, 0);
            }

            color = red << 16 | green << 8 | blue;
            break;
        }

        case mve::IMAGE_TYPE_FLOAT:
        {
            mve::FloatImage::ConstPtr img
                = (mve::FloatImage::ConstPtr)this->image;

            float red = img->at(x, y, this->func.red);
            float green = img->at(x, y, this->func.green);
            float blue = img->at(x, y, this->func.blue);

            if (this->func.highlight_values >= 0.0f
                && red >= 0.0f && red <= this->func.highlight_values
                && green >= 0.0f && green <= this->func.highlight_values
                && blue >= 0.0f && blue <= this->func.highlight_values)
            {
                red = 0.5f;
                green = 0.0f;
                blue = 0.5f;
            }
            else if (MATH_ISNAN(red) || MATH_ISNAN(green)
                || MATH_ISNAN(blue) || MATH_ISINF(red)
                || MATH_ISINF(green) || MATH_ISINF(blue))
            {
                red = 1.0f;
                green = 1.0f;
                blue = 0.0f;
            }
            else
            {
                red = this->func.evaluate(red);
                green = this->func.evaluate(green);
                blue = this->func.evaluate(blue);
            }

            unsigned int vr = (unsigned int)(red * 255.0f + 0.5f);
            unsigned int vg = (unsigned int)(green * 255.0f + 0.5f);
            unsigned int vb = (unsigned int)(blue * 255.0f + 0.5f);
            color = vr << 16 | vg << 8 | vb;
            break;
        }

        default:
            color = ((x % 2) ^ (y % 2)) ? 0xff0000 : 0x00ff00;
            break;
    }

    return color;
}

/* ---------------------------------------------------------------- */

void
ImageInspectorWidget::update_value_label (int x, int y)
{
    std::size_t iw = this->image->width();
    std::size_t ic = this->image->channels();
    std::size_t off = y * iw * ic + x * ic;

    QString value_str;
    QString tooltip_str;
    if (this->image->get_type() == mve::IMAGE_TYPE_FLOAT)
    {
        mve::FloatImage::ConstPtr img(this->image);
        for (std::size_t i = 0; i < ic; ++i)
        {
            if (ic <= 4 || i < 3)
                value_str += tr(" %1").arg(img->at(off + i), 0, 'f', 3);
            if (ic > 4 && i == 3)
                value_str += tr(" ...");
            if (ic > 4)
                tooltip_str += tr(" %1").arg(img->at(off + i), 0, 'f', 3);
        }
    }
    else if (this->image->get_type() == mve::IMAGE_TYPE_UINT8)
    {
        mve::ByteImage::ConstPtr img(this->image);
        for (std::size_t i = 0; i < ic; ++i)
        {
            if (ic <= 4 || i < 3)
                value_str += tr(" %1").arg((int)img->at(off + i));
            if (ic > 4 && i == 3)
                value_str += tr(" ...");
            if (ic > 4)
                tooltip_str += tr(" %1").arg((int)img->at(off + i));
        }
    }

    this->label_values->setText(value_str);
    this->label_values->setToolTip(tooltip_str);
}

/* ---------------------------------------------------------------- */

void
ImageInspectorWidget::reset_images (void)
{
    int size1 = MAGNIFY_SMALL_PATCH * MAGNIFY_SMALL_SCALE;
    int size2 = MAGNIFY_MEDIUM_PATCH * MAGNIFY_MEDIUM_SCALE;
    int size3 = MAGNIFY_LARGE_PATCH * MAGNIFY_LARGE_SCALE;
    QImage image1(size1, size1, QImage::Format_RGB32);
    QImage image2(size2, size2, QImage::Format_RGB32);
    QImage image3(size3, size3, QImage::Format_RGB32);
    image1.fill(0xff00ff);
    image2.fill(0xff00ff);
    image3.fill(0xff00ff);

    this->image_widget1->setPixmap(QPixmap::fromImage(image1));
    this->image_widget2->setPixmap(QPixmap::fromImage(image2));
    this->image_widget3->setPixmap(QPixmap::fromImage(image3));

    this->label_clickpos->setText(tr("--"));
    this->label_values->setText(tr("--"));
    this->inspect_x = -1;
    this->inspect_y = -1;
}

/* ---------------------------------------------------------------- */

void
ImageInspectorWidget::on_large_image_clicked (int x, int y, QMouseEvent* ev)
{
    this->image_click(x, y, ev, MAGNIFY_LARGE_SCALE, MAGNIFY_LARGE_PATCH);
}

/* ---------------------------------------------------------------- */

void
ImageInspectorWidget::on_medium_image_clicked (int x, int y, QMouseEvent* ev)
{
    this->image_click(x, y, ev, MAGNIFY_MEDIUM_SCALE, MAGNIFY_MEDIUM_PATCH);
}

/* ---------------------------------------------------------------- */

void
ImageInspectorWidget::on_small_image_clicked (int x, int y, QMouseEvent* ev)
{
    this->image_click(x, y, ev, MAGNIFY_SMALL_SCALE, MAGNIFY_SMALL_PATCH);
}

/* ---------------------------------------------------------------- */

void
ImageInspectorWidget::image_click (int x, int y,
    QMouseEvent* ev, int scale, int size)
{
    if (this->inspect_x < 0 || this->inspect_y < 0 || this->image.get() == 0)
        return;

    //std::cout << "Click at " << x << "," << y << std::endl;

    int off_x = (x / scale) - size / 2;
    int off_y = (y / scale) - size / 2;

    if (ev->buttons() & Qt::RightButton)
    {
        this->magnify(this->inspect_x + off_x, this->inspect_y + off_y);
    }
    else
    {
        this->update_value_label(this->inspect_x + off_x,
            this->inspect_y + off_y);
    }
}
