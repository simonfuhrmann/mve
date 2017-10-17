/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <limits>
#include <iostream>
#include <cmath>
#include <QButtonGroup>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QRadioButton>

#include "util/timer.h"
#include "util/strings.h"
#include "mve/image.h"
#include "mve/image_tools.h"

#include "tonemapping.h"

#define SIGINT_RED (1 << 29)
#define SIGINT_GREEN (1 << 30)
#define SIGINT_BLUE (1 << 31)

bool
is_on_handle (int handle_pos, int pos)
{
    if (pos <= handle_pos + 2 && pos >= handle_pos - 2)
        return true;
    return false;
}

bool
is_between_handles (int handle_1, int handle_2, int pos)
{
    if (pos > handle_1 + 2 && pos < handle_2 - 2)
        return true;
    return false;
}

ToneMappingHistogram::ToneMappingHistogram (void)
{
    this->mapped_left = 0.0f;
    this->mapped_right = 1.0f;
    this->move_left_handle = false;
    this->move_right_handle = false;
    this->move_mapping_area = false;

    this->timer.setSingleShot(true);
    this->connect(&this->timer, SIGNAL(timeout()),
        this, SLOT(on_timer_expired()));

    this->setMouseTracking(true);
    this->update();
}

void
ToneMappingHistogram::mouseMoveEvent (QMouseEvent* event)
{
    int const mouse_x = event->pos().x();
    int const x_start = this->rect().left() + 1;
    int const x_end = this->rect().right() - 1;

    /* Handle changes to the mapping area. */
    if (this->move_left_handle)
    {
        this->mapped_left = std::max(0.0f, std::min(this->mapped_right - 0.05f,
            static_cast<float>(mouse_x - x_start) / (x_end - x_start)));
        this->update();
        this->timer.start(250);
    }
    if (this->move_right_handle)
    {
        this->mapped_right = std::max(this->mapped_left + 0.05f, std::min(1.0f,
            static_cast<float>(mouse_x - x_start) / (x_end - x_start)));
        this->update();
        this->timer.start(250);
    }
    if (this->move_mapping_area)
    {
        int dist = mouse_x - this->move_mapping_area_start;
        this->move_mapping_area_start = mouse_x;
        float fdist = dist / static_cast<float>(x_end - x_start);
        fdist = std::max(-this->mapped_left, std::min(1.0f - this->mapped_right, fdist));
        this->mapped_left += fdist;
        this->mapped_right += fdist;
        this->update();
        this->timer.start(250);
    }

    /* Compute x position of left and right tone mapping region. */
    int const mapping_x1 = x_start + (x_end - x_start) * this->mapped_left;
    int const mapping_x2 = x_start + (x_end - x_start) * this->mapped_right;

    /* Determine cursor. */
    Qt::CursorShape cursor = Qt::ArrowCursor;
    if (is_on_handle(mapping_x1, mouse_x) || is_on_handle(mapping_x2, mouse_x)
        || this->move_left_handle || this->move_right_handle)
        cursor = Qt::SizeHorCursor;
    else if (is_between_handles(mapping_x1, mapping_x2, mouse_x)
        || this->move_mapping_area)
    {
        if (event->buttons() & Qt::LeftButton)
            cursor = Qt::ClosedHandCursor;
        else
            cursor = Qt::OpenHandCursor;
    }
    this->setCursor(cursor);
}

void
ToneMappingHistogram::mousePressEvent (QMouseEvent* event)
{
    int const mouse_x = event->pos().x();
    int const x_start = this->rect().left() + 1;
    int const x_end = this->rect().right() - 1;
    int const mapping_x1 = x_start + (x_end - x_start) * this->mapped_left;
    int const mapping_x2 = x_start + (x_end - x_start) * this->mapped_right;
    if (is_on_handle(mapping_x1, mouse_x))
        this->move_left_handle = true;
    else if (is_on_handle(mapping_x2, mouse_x))
        this->move_right_handle = true;
    else if (is_between_handles(mapping_x1, mapping_x2, mouse_x))
    {
        this->move_mapping_area = true;
        this->move_mapping_area_start = mouse_x;
    }
    this->mouseMoveEvent(event);
}

void
ToneMappingHistogram::mouseReleaseEvent (QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton)
        return;
    this->move_left_handle = false;
    this->move_right_handle = false;
    this->move_mapping_area = false;
    this->mouseMoveEvent(event);
}

void
ToneMappingHistogram::paintEvent (QPaintEvent* /*event*/)
{
    //https://www.assembla.com/code/histogram/subversion/nodes/2/trunk/Histogram.cpp

    QRect const view_port = this->rect();
    int const x_start = view_port.left() + 1;
    int const x_end = view_port.right() - 1;
    int const y_start = view_port.top() + 1;
    int const y_end = view_port.bottom() - 10;
    int const height = view_port.height();

    /* Setup pen (lines), brush (areas) and the painter itself. */
    QPen pen;
    pen.setColor("#ffffff");
    pen.setStyle(Qt::SolidLine);
    QBrush brush;
    brush.setStyle(Qt::SolidPattern);
    brush.setColor("#ffffff");
    QPainter painter(this);
    painter.setPen(pen);
    painter.setBrush(brush);

    /* Clear the background. */
    painter.drawRect(view_port);

    /* Find maximum height in histogram. */
    int max_height = 0;
    for (std::size_t i = 0; i < this->bins.size(); ++i)
        max_height = std::max(max_height, this->bins[i]);

    /* Create polygon to draw histogram. */
    QPolygon poly;
    poly << QPoint(x_end, y_end) << QPoint(x_start, y_end);
    for (std::size_t i = 0; i < this->bins.size(); ++i)
    {
        float lx_pos = static_cast<float>(i) / (this->bins.size() - 1);
        float ly_pos = static_cast<float>(this->bins[i]) / max_height;
        float x_pos = x_start + (x_end - x_start) * lx_pos;
        float y_pos = y_end - (y_end - y_start - 2) * ly_pos;
        poly << QPoint(x_pos, y_pos);
    }

    /* Draw polygon with fancy linear gradient. */
    QLinearGradient gradient(0, 0, 0, height);
    gradient.setColorAt(0.2, "#ffcccc");
    gradient.setColorAt(1.0, "#663333");
    pen.setColor("#996666");
    painter.setPen(pen);
    painter.setBrush(gradient);
    painter.drawPolygon(poly);

    /* Draw mapping region. */
    pen.setColor("#0000ff");
    pen.setStyle(Qt::SolidLine);
    brush.setColor("#0000ff");
    painter.setPen(pen);
    painter.setBrush(brush);

    int const mapping_x1 = x_start + (x_end - x_start) * this->mapped_left;
    int const mapping_x2 = x_start + (x_end - x_start) * this->mapped_right;
    painter.drawRect(QRect(mapping_x1, y_end + 2,
        mapping_x2 - mapping_x1, view_port.bottom() - y_end - 3));
}

void
ToneMappingHistogram::set_bins (std::vector<int> const& bins)
{
    this->bins = bins;
    this->update();
}

void
ToneMappingHistogram::clear (void)
{
    this->bins.clear();
    this->mapped_left = 0.0f;
    this->mapped_right = 1.0f;
    this->update();
}

int
ToneMappingHistogram::preferred_num_bins (void) const
{
    return this->rect().width() - 2;
}

QSize
ToneMappingHistogram::sizeHint (void) const
{
    return QSize(150, 100);
}

void
ToneMappingHistogram::on_timer_expired()
{
    emit this->mapping_area_changed(this->mapped_left, this->mapped_right);
}

void
ToneMappingHistogram::get_mapping_range (float* map_min, float* map_max)
{
    *map_min = this->mapped_left;
    *map_max = this->mapped_right;
}

/* ---------------------------------------------------------------- */

ToneMapping::ToneMapping (void)
    : ignore_zeros(true)
{
    this->histogram = new ToneMappingHistogram();
    this->ignore_zeros_checkbox = new QCheckBox("Ignore zeros");
    this->ignore_zeros_checkbox->setChecked(this->ignore_zeros);
    this->gamma_label = new QLabel("1.00");
    this->gamma_slider = new QSlider();
    this->gamma_slider->setRange(-20, 20);
    this->gamma_slider->setTickInterval(10);
    this->gamma_slider->setTickPosition(QSlider::TicksBelow);
    this->gamma_slider->setOrientation(Qt::Horizontal);
    this->highlight_checkbox = new QCheckBox("Highlight values");
    this->highlight_checkbox->setChecked(true);
    this->highlight_label = new QLabel("<= 0.00");
    this->highlight_slider = new QSlider(Qt::Horizontal);
    this->highlight_slider->setRange(0, 1000);
    this->highlight_slider->setTickInterval(100);
    this->highlight_slider->setTickPosition(QSlider::TicksBelow);
    this->highlight_slider->setOrientation(Qt::Horizontal);
    this->channel_grid = new QGridLayout();
    this->channel_mapper = new QSignalMapper();
    this->timer.setSingleShot(true);
    this->reset();
    this->setEnabled(false);

    QHBoxLayout* gamma_box = new QHBoxLayout();
    gamma_box->addWidget(new QLabel("Gamma Exponent"), 1);
    gamma_box->addWidget(this->gamma_label, 0);

    QHBoxLayout* highlight_box = new QHBoxLayout();
    highlight_box->addWidget(this->highlight_checkbox, 1);
    highlight_box->addWidget(this->highlight_label, 0);

    QVBoxLayout* main_layout = new QVBoxLayout();
    main_layout->setSpacing(0);
    main_layout->addWidget(this->histogram);
    main_layout->addSpacing(10);
    main_layout->addWidget(this->ignore_zeros_checkbox);
    main_layout->addSpacing(10);
    main_layout->addLayout(gamma_box);
    main_layout->addWidget(this->gamma_slider);
    main_layout->addSpacing(10);
    main_layout->addLayout(highlight_box);
    main_layout->addWidget(this->highlight_slider);
    main_layout->addSpacing(10);
    main_layout->addLayout(this->channel_grid);
    main_layout->addStretch(1);
    this->setLayout(main_layout);

    this->connect(this->ignore_zeros_checkbox, SIGNAL(toggled(bool)),
        this, SLOT(on_ignore_zeroes_changed()));
    this->connect(this->gamma_slider, SIGNAL(valueChanged(int)),
        this, SLOT(on_gamma_value_changed()));
    this->connect(this->highlight_slider, SIGNAL(valueChanged(int)),
        this, SLOT(on_highlight_value_changed()));
    this->connect(this->histogram, SIGNAL(mapping_area_changed(float, float)),
        this, SLOT(on_mapping_area_changed(float, float)));
    this->connect(this->channel_mapper, SIGNAL(mapped(int)),
        this, SLOT(on_channels_changed(int)));
    this->connect(this->highlight_checkbox, SIGNAL(toggled(bool)),
        this, SLOT(on_update_tone_mapping()));
    this->connect(&this->timer, SIGNAL(timeout()),
        this, SLOT(on_update_tone_mapping()));
}

void
ToneMapping::reset (void)
{
    this->histogram->clear();
    this->image.reset();
    this->image_vmin = 0.0f;
    this->image_vmax = 0.0f;
    std::fill(this->channel_assignment, this->channel_assignment + 3, 0);
    // TODO: Clear/disable widgets?
}

void
ToneMapping::on_ignore_zeroes_changed (void)
{
    this->ignore_zeros = this->ignore_zeros_checkbox->isChecked();
    this->setup_histogram();
    emit this->tone_mapping_changed();
}

void
ToneMapping::on_gamma_value_changed (void)
{
    std::string exp_string = util::string::get_fixed(gamma_from_slider(), 2);
    this->gamma_label->setText(exp_string.c_str());
    this->timer.start(400);
}

void
ToneMapping::on_highlight_value_changed (void)
{
    std::string fstr = util::string::get_fixed(highlight_from_slider(), 2);
    this->highlight_label->setText(tr("<= %1").arg(fstr.c_str()));
    this->timer.start(400);
}

void
ToneMapping::on_mapping_area_changed (float /*start*/, float /*end*/)
{
    this->timer.start(250);
}

float
ToneMapping::gamma_from_slider (void) const
{
    float fvalue = static_cast<float>(this->gamma_slider->value()) / 10.0f;
    float exponent = std::pow(2.0f, fvalue);
    return exponent;
}

void
ToneMapping::on_update_tone_mapping (void)
{
    emit this->tone_mapping_changed();
}

float
ToneMapping::highlight_from_slider (void) const
{
    if (this->highlight_checkbox->isChecked())
        return static_cast<float>(this->highlight_slider->value()) / 1000.0f;
    else
        return -std::numeric_limits<float>::max();
}

void
ToneMapping::setup_histogram (void)
{
    /* Don't compute histogram for byte images for performance reasons. */
    if (this->image->get_type() == mve::IMAGE_TYPE_UINT8)
        return;
    if (this->image->get_type() != mve::IMAGE_TYPE_FLOAT)
        throw std::invalid_argument("Unsupported image type.");

    mve::FloatImage::ConstPtr fimg_ptr =
        std::dynamic_pointer_cast<mve::FloatImage const>(this->image);
    mve::FloatImage const& fimg = *fimg_ptr;

    /* Find min/max value of the image. */
    float min = std::numeric_limits<float>::max();
    float max = -std::numeric_limits<float>::max();
    for (float const *p = fimg.begin(); p != fimg.end(); ++p)
    {
        float const v = *p;
        if (std::isfinite(v) && (!this->ignore_zeros || v != 0.0f))
        {
            min = std::min(min, v);
            max = std::max(max, v);
        }
    }

    this->image_vmin = min;
    this->image_vmax = max;

    /* Compute image histogram. */
    int const num_bins = this->histogram->preferred_num_bins();
    std::vector<int> bins(num_bins, 0);
    float const image_range = this->image_vmax - this->image_vmin;
    /* Only build a histogram if the image is not constant. */
    if (this->image_vmax != this->image_vmin && std::isfinite(image_range))
    {
        for (float const* ptr = fimg.begin(); ptr != fimg.end(); ++ptr)
        {
            float const value = *ptr;
            if (value < this->image_vmin || value > this->image_vmax
                || !std::isfinite(value))
                continue;
            float normalized = (*ptr - this->image_vmin) / image_range;
            int bin = std::log10(1.0f + 9.0f * normalized) * (num_bins - 1);
            bins[bin] += 1;
        }
    }
    this->histogram->set_bins(bins);
}

void
ToneMapping::set_image (mve::ImageBase::ConstPtr img)
{
    this->image = img;

    /* Convert 16 bit image to float. */
    if (this->image->get_type() == mve::IMAGE_TYPE_UINT16)
        this->image = mve::image::type_to_type_image<uint16_t, float>
            (std::dynamic_pointer_cast<mve::RawImage const>(this->image));

    /* Compute image histogram. */
    this->histogram->clear();
    this->setup_histogram();

    /*
     * Create channel assignment UI.
     */

    /* Clear current contents. */
    while (this->channel_grid->count())
    {
        QWidget* w = this->channel_grid->takeAt(0)->widget();
        this->channel_grid->removeWidget(w);
        delete w;
    }

    int channels = img->channels();
    if (channels <= 0)
        return;

    /* Create table for channel assignment. */
    this->channel_grid->addWidget(new QLabel(tr("R")), 0, 1);
    this->channel_grid->addWidget(new QLabel(tr("G")), 0, 2);
    this->channel_grid->addWidget(new QLabel(tr("B")), 0, 3);

    QButtonGroup* grp_r = new QButtonGroup(this);
    QButtonGroup* grp_g = new QButtonGroup(this);
    QButtonGroup* grp_b = new QButtonGroup(this);
    for (int i = 0; i < std::min(10, channels); ++i)
    {
        QPushButton* ch_but = new QPushButton(tr("Channel %1").arg(i));
        ch_but->setFlat(true);

        QRadioButton* ch_r = new QRadioButton();
        QRadioButton* ch_g = new QRadioButton();
        QRadioButton* ch_b = new QRadioButton();
        grp_r->addButton(ch_r);
        grp_g->addButton(ch_g);
        grp_b->addButton(ch_b);

        this->channel_grid->addWidget(ch_but, i + 1, 0);
        this->channel_grid->addWidget(ch_r, i + 1, 1);
        this->channel_grid->addWidget(ch_g, i + 1, 2);
        this->channel_grid->addWidget(ch_b, i + 1, 3);

        this->connect(ch_r, SIGNAL(clicked()),
            this->channel_mapper, SLOT(map()));
        this->connect(ch_g, SIGNAL(clicked()),
            this->channel_mapper, SLOT(map()));
        this->connect(ch_b, SIGNAL(clicked()),
            this->channel_mapper, SLOT(map()));
        this->connect(ch_but, SIGNAL(clicked()), ch_r, SLOT(click()));
        this->connect(ch_but, SIGNAL(clicked()), ch_g, SLOT(click()));
        this->connect(ch_but, SIGNAL(clicked()), ch_b, SLOT(click()));
        this->channel_mapper->setMapping(ch_r, SIGINT_RED | i);
        this->channel_mapper->setMapping(ch_g, SIGINT_GREEN | i);
        this->channel_mapper->setMapping(ch_b, SIGINT_BLUE | i);

        if (i == 0)
        {
            ch_r->setChecked(true);
            ch_g->setChecked(true);
            ch_b->setChecked(true);
            std::fill(channel_assignment, channel_assignment + 3, i);
        }

        if (i == 1 && channels >= 3)
        {
            ch_g->setChecked(true);
            channel_assignment[1] = i;
        }

        if (i == 2 && channels >= 3)
        {
            ch_b->setChecked(true);
            channel_assignment[2] = i;
        }
    }

    /* Limit channel assignment to 10 channels. */
    if (channels > 10)
    {
        QLabel* more_label = new QLabel
            (tr("<omitted %1 channels>").arg(channels - 10));
        more_label->setAlignment(Qt::AlignCenter);
        this->channel_grid->addWidget(more_label, 11, 0, 1, -1);
    }
}

mve::ByteImage::ConstPtr
ToneMapping::render (void)
{
    if (this->image->get_type() == mve::IMAGE_TYPE_UINT8)
        return std::dynamic_pointer_cast<mve::ByteImage const>(this->image);

    //util::WallTimer timer;
    //std::cout << "Rendering..." << std::flush;
    int const width = this->image->width();
    int const height = this->image->height();
    int const chans = this->image->channels();
    mve::ByteImage::Ptr ret = mve::ByteImage::create(width, height, 3);

    if (this->image->get_type() != mve::IMAGE_TYPE_FLOAT)
    {
        std::cerr << "Warning: Unsupported image type!" << std::endl;
        return ret;
    }

    float const gamma_exp = this->gamma_from_slider();
    float const highlight = this->highlight_from_slider();
    float const image_range = this->image_vmax - this->image_vmin;

    float map_min, map_max;
    this->histogram->get_mapping_range(&map_min, &map_max);
    float linear_min = (std::pow(10.0f, map_min) - 1.0f) / 9.0f;
    float linear_max = (std::pow(10.0f, map_max) - 1.0f) / 9.0f;
    float min_value = this->image_vmin + linear_min * image_range;
    float max_value = this->image_vmin + linear_max * image_range;

    mve::FloatImage::ConstPtr fimg = std::dynamic_pointer_cast
        <mve::FloatImage const>(this->image);
    uint8_t* out_ptr = ret->begin();
    for (float const* px = fimg->begin();
        px != fimg->end(); px += chans, out_ptr += 3)
    {
        bool all_below_highlight = true;
        bool has_bad_value = false;
        for (int c = 0; !has_bad_value && c < 3; ++c)
        {
            float value = px[this->channel_assignment[c]];
            has_bad_value = has_bad_value || !std::isfinite(value);
            all_below_highlight = all_below_highlight
                && value <= highlight && value >= 0.0f;

            value = (value - min_value) / (max_value - min_value);
            value = std::max(0.0f, std::min(1.0f, value));
            if (gamma_exp != 1.0f)
                value = std::pow(value, gamma_exp);
            out_ptr[c] = static_cast<uint8_t>(value * 255.0f + 0.5f);
        }

        if (has_bad_value)
        {
            out_ptr[0] = 255;
            out_ptr[1] = 255;
            out_ptr[2] = 0;
        }
        else if (all_below_highlight)
        {
            out_ptr[0] = 127;
            out_ptr[1] = 0;
            out_ptr[2] = 127;
        }
    }
    //std::cout << " took " << timer.get_elapsed() << "ms." << std::endl;

    return ret;
}

void
ToneMapping::on_channels_changed (int mask)
{
    if (mask & SIGINT_RED)
        this->channel_assignment[0] = mask ^ SIGINT_RED;
    if (mask & SIGINT_GREEN)
        this->channel_assignment[1] = mask ^ SIGINT_GREEN;
    if (mask & SIGINT_BLUE)
        this->channel_assignment[2] = mask ^ SIGINT_BLUE;
    this->timer.start(50);
}
