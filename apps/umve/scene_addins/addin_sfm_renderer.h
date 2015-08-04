#ifndef UMVE_ADDIN_SFM_RENDERER_HEADER
#define UMVE_ADDIN_SFM_RENDERER_HEADER

#include <QCheckBox>

#include "ogl/mesh_renderer.h"

#include "scene_addins/addin_base.h"

class AddinSfmRenderer : public AddinBase
{
    Q_OBJECT

public:
    AddinSfmRenderer (void);
    QWidget* get_sidebar_widget (void);

protected:
    void paint_impl (void);
    void create_renderer (bool raise_error_on_failure);

protected slots:
    void reset_scene_bundle (void);

private:
    QCheckBox* render_cb;
    bool first_create_renderer;
    ogl::MeshRenderer::Ptr sfm_renderer;
};

#endif // UMVE_ADDIN_SFM_RENDERER_HEADER
