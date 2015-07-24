/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef TONE_MAPPING_HEADER
#define TONE_MAPPING_HEADER

#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QSignalMapper>
#include <QSlider>
#include <QTimer>
#include <QWidget>

#include "mve/image.h"

class ToneMappingHistogram : public QWidget
{
    Q_OBJECT

public:
    ToneMappingHistogram (void);
    virtual ~ToneMappingHistogram (void) {}

    /* Sets the histogram bins. */
    void set_bins (std::vector<int> const& bins);
    /* Clears the histogram bins. */
    void clear (void);
    /* Returns the mapping in range [0, 1]. */
    void get_mapping_range (float* map_min, float* map_max);
    /* Returns the preferred number of bins depending on width. */
    int preferred_num_bins (void) const;

    QSize sizeHint (void) const;

signals:
    void mapping_area_changed (float start, float end);

private slots:
    void on_timer_expired (void);

private:
    void paintEvent (QPaintEvent* event);
    void mouseMoveEvent (QMouseEvent* event);
    void mousePressEvent (QMouseEvent* event);
    void mouseReleaseEvent (QMouseEvent* event);

private:
    QTimer timer;
    std::vector<int> bins;
    float mapped_left;
    float mapped_right;
    bool move_left_handle;
    bool move_right_handle;
    bool move_mapping_area;
    int move_mapping_area_start;
};

/* ---------------------------------------------------------------- */

class ToneMapping : public QWidget
{
    Q_OBJECT

public:
    ToneMapping (void);
    virtual ~ToneMapping (void) {}
    void reset (void);
    void set_image (mve::ImageBase::ConstPtr img);
    mve::ByteImage::ConstPtr render (void);

signals:
    void tone_mapping_changed (void);

private slots:
    void on_gamma_value_changed (void);
    void on_highlight_value_changed (void);
    void on_mapping_area_changed (float start, float end);
    void on_channels_changed (int mask);
    void on_update_tone_mapping (void);
    void on_ignore_zeroes_changed (void);

private:
    float gamma_from_slider (void) const;
    float highlight_from_slider (void) const;
    void setup_histogram (void);

private:
    QTimer timer;
    ToneMappingHistogram* histogram;
    QCheckBox *ignore_zeros_checkbox;
    QSlider* gamma_slider;
    QLabel* gamma_label;
    QCheckBox* highlight_checkbox;
    QSlider* highlight_slider;
    QLabel* highlight_label;
    QGridLayout* channel_grid;
    QSignalMapper* channel_mapper;

    bool ignore_zeros;
    mve::ImageBase::ConstPtr image;
    float image_vmin;
    float image_vmax;
    int channel_assignment[3];
};

#endif // TONE_MAPPING_HEADER
