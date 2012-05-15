#ifndef TRANSFER_FUNCTION_HEADER
#define TRANSFER_FUNCTION_HEADER

#include <QtGui>

struct TransferFunction
{
    /* Value transfer settings. */
    float clamp_min;
    float clamp_max;
    float zoom;
    float gamma;

    /* Channel assignments. */
    int red;
    int green;
    int blue;

    /* Special values. */
    float highlight_values;

    TransferFunction (void);
    float evaluate (float value);
};

/* ---------------------------------------------------------------- */

class TransferFunctionWidget : public QWidget
{
    Q_OBJECT

private slots:
    void on_slider_changed (void);
    void on_slider_released (void);
    void on_assignment_changed (int mask);
    void update_sliders (void);
    void on_highlight_values_changed (void);

private:
    TransferFunction func;
    QSignalMapper* ca_mapper;

    QSlider* zoom_slider;
    QSlider* gamma_slider;
    QSlider* minvalue_slider;
    QSlider* maxvalue_slider;
    QSlider* highlight_slider;

    QLabel* zoom_label;
    QLabel* gamma_label;
    QLabel* minvalue_label;
    QLabel* maxvalue_label;

    QCheckBox* fix_clamp_slider;
    QCheckBox* highlight_values;
    QGridLayout* channel_grid;

signals:
    void function_changed (TransferFunction func);

public:
    TransferFunctionWidget (void);

    void set_minmax_range (float min, float max);
    void set_clamp_range (float min, float max);
    void set_color_assignment (int channels);
    void show_minmax_sliders (bool value);

    TransferFunction const& get_function (void) const;
};

/* ---------------------------------------------------------------- */

inline
TransferFunction::TransferFunction (void)
{
    this->clamp_min = 0.0f;
    this->clamp_max = 1.0f;
    this->zoom = 1.0f;
    this->gamma = 1.0f;

    this->red = 0;
    this->green = 1;
    this->blue = 2;

    this->highlight_values = 0.0f;
}

inline TransferFunction const&
TransferFunctionWidget::get_function (void) const
{
    return this->func;
}

#endif /* TRANSFER_FUNCTION_HEADER */
