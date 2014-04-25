#ifndef UMVE_ADDIN_FRUSTA_RENDERER_HEADER
#define UMVE_ADDIN_FRUSTA_RENDERER_HEADER

#include <QWidget>
#include <QCheckBox>
#include <QSlider>
#include <QFormLayout>

#include "mve/view.h"
#include "ogl/mesh_renderer.h"

#include "scene_addins/addin_base.h"

class AddinFrustaRenderer : public AddinBase
{
    Q_OBJECT

public:
    AddinFrustaRenderer (void);
    QWidget* get_sidebar_widget (void);

protected:
    void create_frusta_renderer (void);
    void create_viewdir_renderer (void);
    void paint_impl (void);

protected slots:
    void reset_viewdir_renderer (void);
    void reset_frusta_renderer (void);

private:
    QFormLayout* render_frusta_form;
    QCheckBox* render_frusta_cb;
    QCheckBox* render_viewdir_cb;
    QSlider* frusta_size_slider;
    mve::View::Ptr last_view;
    ogl::MeshRenderer::Ptr frusta_renderer;
    ogl::MeshRenderer::Ptr viewdir_renderer;
};

#endif // UMVE_ADDIN_FRUSTA_RENDERER_HEADER
