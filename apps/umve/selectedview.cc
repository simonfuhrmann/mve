#include <QVBoxLayout>

#include "selectedview.h"

SelectedView::SelectedView (void)
{
    this->viewname = new QLabel();
    this->image = new QLabel();
    this->image->setBaseSize(QSize(50, 50));
    this->viewinfo = new QLabel();
    //this->embeddings = new QComboBox();

    QVBoxLayout* label_box = new QVBoxLayout();
    label_box->setSpacing(0);
    label_box->addWidget(this->viewname, 0);
    label_box->addWidget(this->viewinfo, 0);
    //label_box->addStretch(1);
    //label_box->addWidget(this->embeddings);

    QHBoxLayout* image_labels_box = new QHBoxLayout();
    image_labels_box->addWidget(this->image, 0);
    image_labels_box->addLayout(label_box, 1);

    QVBoxLayout* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(QMargins(0,0,0,0));
    main_layout->addLayout(image_labels_box);

    this->reset_view();
}

/* ---------------------------------------------------------------- */

void
SelectedView::set_view (mve::View::Ptr view)
{
    if (view == NULL)
    {
        this->reset_view();
        return;
    }

    this->view = view;
    this->viewname->setText(("View: " + view->get_name()).c_str());
    this->viewinfo->setText((util::string::get
        (view->num_embeddings()) + " embeddings").c_str());

    mve::ByteImage::Ptr img(view->get_byte_image("thumbnail"));
    if (img != NULL)
    {
        std::size_t iw(img->width());
        std::size_t ih(img->height());
        QImage qimg(iw, ih, QImage::Format_RGB32);
        for (std::size_t y = 0; y < ih; ++y)
            for (std::size_t x = 0; x < iw; ++x)
            {
                unsigned int vr = (unsigned int)(img->at(x, y, 0));
                unsigned int vg = (unsigned int)(img->at(x, y, 1));
                unsigned int vb = (unsigned int)(img->at(x, y, 2));
                unsigned int rgb = vr << 16 | vg << 8 | vb;
                qimg.setPixel(x, y, rgb);
            }

        this->image->setPixmap(QPixmap::fromImage(qimg));
    }

    emit this->view_selected(this->view);
}

/* ---------------------------------------------------------------- */

void
SelectedView::fill_embeddings (QComboBox& cb, mve::ImageType type,
    std::string const& default_name) const
{
    if (this->view == NULL)
        return;

    cb.clear();
    cb.addItem("<none>");

    /* Read embedding names from view and sort. */
    typedef std::pair<std::string, mve::ImageType> ProxyType;
    typedef std::vector<ProxyType> ProxyVector;
    mve::View::Proxies const& proxies(this->view->get_proxies());
    ProxyVector pvec;
    for (std::size_t i = 0; i < proxies.size(); ++i)
        pvec.push_back(std::make_pair(proxies[i].name,
            proxies[i].get_type()));
    std::sort(pvec.begin(), pvec.end());

    /* Add views to combo box. */
    for (std::size_t i = 0; i < pvec.size(); ++i)
    {
        ProxyType const& p(pvec[i]);
        if (p.second != type)
            continue;

        cb.addItem(p.first.c_str());
        if (p.first == default_name)
            cb.setCurrentIndex(cb.count() - 1);
    }
}

/* ---------------------------------------------------------------- */

void
SelectedView::reset_view (void)
{
    this->view.reset();
    this->viewname->setText("<no view selected>");
    this->viewinfo->setText("");
    this->image->setPixmap(QPixmap(":/images/icon_broken.svg"));
    emit this->view_selected(this->view);
}
