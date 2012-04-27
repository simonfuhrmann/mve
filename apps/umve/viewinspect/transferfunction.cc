#include <iostream>
#include <stdexcept>
#include <cmath>

#include "guihelpers.h"
#include "transferfunction.h"

#define SIGINT_RED (1 << 29)
#define SIGINT_GREEN (1 << 30)
#define SIGINT_BLUE (1 << 31)

float
TransferFunction::evaluate (float value)
{
    /* Scale to [0, 1] */
    value = (value - this->clamp_min) / (this->clamp_max - this->clamp_min);
    value = std::min(1.0f, std::max(0.0f, value));

    /* Apply zoom. */
    value = (value - 0.5f) * this->zoom + 0.5f;
    value = std::min(1.0f, std::max(0.0f, value));

    /* Apply gamma. */
    value = (this->gamma == 1.0f ? value : std::pow(value, this->gamma));
    value = std::min(1.0f, std::max(0.0f, value));

    return value;
}

/* ---------------------------------------------------------------- */

TransferFunctionWidget::TransferFunctionWidget (void)
{
    this->zoom_slider = new QSlider(Qt::Horizontal);
    this->zoom_slider->setRange(100, 1000);
    this->gamma_slider = new QSlider(Qt::Horizontal);
    this->gamma_slider->setRange(0, 300);
    this->minvalue_slider = new QSlider(Qt::Horizontal);
    this->minvalue_slider->setRange(0, 100000);
    this->maxvalue_slider = new QSlider(Qt::Horizontal);
    this->maxvalue_slider->setRange(0, 100000);

    this->zoom_label = new QLabel();
    this->gamma_label = new QLabel();
    this->minvalue_label = new QLabel();
    this->maxvalue_label = new QLabel();
    this->fix_clamp_slider = new QCheckBox("Fix clamp sliders");
    this->fix_clamp_slider->setChecked(false);
    this->highlight_zeros = new QCheckBox("Highlight zeros");
    this->highlight_zeros->setChecked(this->func.highlight_zeros);

    this->channel_grid = new QGridLayout();
    this->ca_mapper = new QSignalMapper();

    /* Create transfer function control widgets. */
    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(this->zoom_label);
    layout->addWidget(this->zoom_slider);
    layout->addWidget(this->gamma_label);
    layout->addWidget(this->gamma_slider);
    layout->addWidget(this->minvalue_label);
    layout->addWidget(this->minvalue_slider);
    layout->addWidget(this->maxvalue_label);
    layout->addWidget(this->maxvalue_slider);
    layout->addWidget(this->fix_clamp_slider);
    layout->addWidget(this->highlight_zeros);
    layout->addWidget(get_separator());
    layout->addSpacerItem(new QSpacerItem(0, 10));
    layout->addLayout(this->channel_grid);
    layout->addSpacerItem(new QSpacerItem(0, 0,
        QSizePolicy::Minimum, QSizePolicy::Expanding));

    this->update_sliders();
    this->on_slider_changed();

    this->connect(this->ca_mapper, SIGNAL(mapped(int)),
        this, SLOT(on_assignment_changed(int)));
    this->connect(this->zoom_slider, SIGNAL(valueChanged(int)),
        this, SLOT(on_slider_changed()));
    this->connect(this->gamma_slider, SIGNAL(valueChanged(int)),
        this, SLOT(on_slider_changed()));
    this->connect(this->minvalue_slider, SIGNAL(valueChanged(int)),
        this, SLOT(on_slider_changed()));
    this->connect(this->maxvalue_slider, SIGNAL(valueChanged(int)),
        this, SLOT(on_slider_changed()));

    this->connect(this->zoom_slider, SIGNAL(sliderReleased()),
        this, SLOT(on_slider_released()));
    this->connect(this->gamma_slider, SIGNAL(sliderReleased()),
        this, SLOT(on_slider_released()));
    this->connect(this->minvalue_slider, SIGNAL(sliderReleased()),
        this, SLOT(on_slider_released()));
    this->connect(this->maxvalue_slider, SIGNAL(sliderReleased()),
        this, SLOT(on_slider_released()));

    this->connect(this->highlight_zeros, SIGNAL(toggled(bool)),
        this, SLOT(on_highlight_zeros_changed()));

    this->setLayout(layout);
}

/* ---------------------------------------------------------------- */

void
TransferFunctionWidget::set_color_assignment (int channels)
{
    /* Clear current contents. */
    while (this->channel_grid->count())
    {
        QWidget* w = this->channel_grid->takeAt(0)->widget();
        this->channel_grid->removeWidget(w);
        delete w;
    }

    if (channels <= 0)
        return;

    /* Create table for channel assignment. */
    QLabel* r_label = new QLabel(tr("R"));
    QLabel* g_label = new QLabel(tr("G"));
    QLabel* b_label = new QLabel(tr("B"));

    this->channel_grid->addWidget(r_label, 0, 1);
    this->channel_grid->addWidget(g_label, 0, 2);
    this->channel_grid->addWidget(b_label, 0, 3);

    QButtonGroup* grp_r = new QButtonGroup(this);
    QButtonGroup* grp_g = new QButtonGroup(this);
    QButtonGroup* grp_b = new QButtonGroup(this);

    for (int i = 0; i < channels; ++i)
    {
        QLabel* ch_label = new QLabel(tr("Channel %1").arg(i));

        QRadioButton* ch_r = new QRadioButton();
        QRadioButton* ch_g = new QRadioButton();
        QRadioButton* ch_b = new QRadioButton();

        grp_r->addButton(ch_r);
        grp_g->addButton(ch_g);
        grp_b->addButton(ch_b);

        this->channel_grid->addWidget(ch_label, i + 1, 0);
        this->channel_grid->addWidget(ch_r, i + 1, 1);
        this->channel_grid->addWidget(ch_g, i + 1, 2);
        this->channel_grid->addWidget(ch_b, i + 1, 3);

        this->connect(ch_r, SIGNAL(released()), this->ca_mapper, SLOT(map()));
        this->connect(ch_g, SIGNAL(released()), this->ca_mapper, SLOT(map()));
        this->connect(ch_b, SIGNAL(released()), this->ca_mapper, SLOT(map()));
        this->ca_mapper->setMapping(ch_r, SIGINT_RED | i);
        this->ca_mapper->setMapping(ch_g, SIGINT_GREEN | i);
        this->ca_mapper->setMapping(ch_b, SIGINT_BLUE | i);

        if (i == 0)
        {
            ch_r->setChecked(true); this->func.red = i;
            ch_g->setChecked(true); this->func.green = i;
            ch_b->setChecked(true); this->func.blue = i;
        }

        if (i == 1 && channels >= 3)
        { ch_g->setChecked(true); this->func.green = i; }

        if (i == 2 && channels >= 3)
        { ch_b->setChecked(true); this->func.blue = i; }
    }

    emit this->function_changed(this->func);
}

/* ---------------------------------------------------------------- */

void
TransferFunctionWidget::show_minmax_sliders (bool value)
{
    this->minvalue_slider->setVisible(value);
    this->maxvalue_slider->setVisible(value);
    this->minvalue_label->setVisible(value);
    this->maxvalue_label->setVisible(value);
    this->fix_clamp_slider->setVisible(value);
}

/* ---------------------------------------------------------------- */

void
TransferFunctionWidget::update_sliders (void)
{
    this->minvalue_slider->blockSignals(true);
    this->maxvalue_slider->blockSignals(true);
    this->zoom_slider->blockSignals(true);
    this->gamma_slider->blockSignals(true);

    this->minvalue_slider->setValue(this->func.clamp_min * 1000.0f);
    this->maxvalue_slider->setValue(this->func.clamp_max * 1000.0f);
    this->zoom_slider->setValue(this->func.zoom * 100.0f);
    this->gamma_slider->setValue(this->func.gamma * 100.0f);

    this->minvalue_slider->blockSignals(false);
    this->maxvalue_slider->blockSignals(false);
    this->zoom_slider->blockSignals(false);
    this->gamma_slider->blockSignals(false);

    this->on_slider_changed();
}

/* ---------------------------------------------------------------- */

void
TransferFunctionWidget::on_assignment_changed (int mask)
{
    if (mask & SIGINT_RED)
        this->func.red = mask ^ SIGINT_RED;
    if (mask & SIGINT_GREEN)
        this->func.green = mask ^ SIGINT_GREEN;
    if (mask & SIGINT_BLUE)
        this->func.blue = mask ^ SIGINT_BLUE;

    emit this->function_changed(this->func);
}

/* ---------------------------------------------------------------- */

void
TransferFunctionWidget::on_slider_released (void)
{
    emit this->function_changed(this->func);
}

/* ---------------------------------------------------------------- */

void
TransferFunctionWidget::on_slider_changed (void)
{
    this->func.zoom = (float)this->zoom_slider->value() / 100.0f;
    this->zoom_label->setText(tr("Zoom: %1")
        .arg(this->func.zoom, 0, 'f', 2));

    this->func.gamma = (float)this->gamma_slider->value() / 100.0f;
    this->gamma_label->setText(tr("Gamma: %1")
        .arg(this->func.gamma, 0, 'f', 2));

    this->func.clamp_min = (float)this->minvalue_slider->value() / 1000.0f;
    this->minvalue_label->setText(tr("Clamp min: %1")
        .arg(this->func.clamp_min, 0, 'f', 3));

    this->func.clamp_max = (float)this->maxvalue_slider->value() / 1000.0f;
    this->maxvalue_label->setText(tr("Clamp max: %1")
        .arg(this->func.clamp_max, 0, 'f', 3));
}

/* ---------------------------------------------------------------- */

void
TransferFunctionWidget::on_highlight_zeros_changed (void)
{
    this->func.highlight_zeros = this->highlight_zeros->isChecked();
    emit this->function_changed(this->func);
}

/* ---------------------------------------------------------------- */

void
TransferFunctionWidget::set_minmax_range (float min, float max)
{
    if (this->fix_clamp_slider->isChecked())
    {
        min = std::min(min, this->func.clamp_min);
        max = std::max(max, this->func.clamp_max);
    }

    this->minvalue_slider->setRange((int)(1000.0f * min), (int)(1000.0f * max));
    this->maxvalue_slider->setRange((int)(1000.0f * min), (int)(1000.0f * max));
}

/* ---------------------------------------------------------------- */

void
TransferFunctionWidget::set_clamp_range (float min, float max)
{
    if (this->fix_clamp_slider->isChecked())
        return;

    this->func.clamp_min = min;
    this->func.clamp_max = max;

    this->update_sliders();
}
