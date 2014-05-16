#ifndef UMVE_ADDIN_FRUSTA_SFM_RENDERER_HEADER
#define UMVE_ADDIN_FRUSTA_SFM_RENDERER_HEADER

#include <QWidget>
#include <QCheckBox>
#include <QSlider>
#include <QFormLayout>

#include <vector>

#include "mve/camera.h"
#include "mve/view.h"
#include "ogl/mesh_renderer.h"

#include "scene_addins/addin_base.h"

class AddinFrustaSfmRenderer : public AddinBase
{
    Q_OBJECT

public:
    AddinFrustaSfmRenderer (void);
    QWidget* get_sidebar_widget (void);
    void set_cameras (std::vector<mve::CameraInfo> const& cameras);

protected slots:
    void reset_frusta_renderer (void);
    void create_frusta_renderer (void);
    void paint_impl (void);

private:
    QFormLayout* render_frusta_form;
    QCheckBox* render_frusta_cb;
    QSlider* frusta_size_slider;
    ogl::MeshRenderer::Ptr frusta_renderer;
    std::vector<mve::CameraInfo> cameras;
};

#endif /* UMVE_ADDIN_FRUSTA_SFM_RENDERER_HEADER */
