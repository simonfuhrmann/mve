#ifndef UMVE_SCENECONTEXT_HEADER
#define UMVE_SCENECONTEXT_HEADER

#include <string>

#include "ogl/opengl.h"
#include "ogl/context.h"
#include "ogl/texture.h"
#include "ogl/vertexarray.h"
#include "ogl/shaderprogram.h"

#include "mve/scene.h"
#include "mve/view.h"
#include "mve/image.h"

#include "selectedview.h"
#include "camerasequence.h"
#include "meshlist.h"
#include "guihelpers.h"
#include "guicontext.h"

class SceneContext : public GuiContext, public ogl::CameraTrackballContext
{
    Q_OBJECT

private:
    mve::Scene::Ptr scene;
    SelectedView* view;
    QMeshList* meshlist;

    /* Misc renderer. */
    ogl::VertexArray::Ptr axis_renderer;
    ogl::MeshRenderer::Ptr sfm_renderer;
    ogl::MeshRenderer::Ptr frusta_renderer;
    ogl::MeshRenderer::Ptr current_frustum_renderer;

    /* Shader. */
    ogl::ShaderProgram::Ptr surface_shader;
    ogl::ShaderProgram::Ptr wireframe_shader;
    ogl::ShaderProgram::Ptr texture_shader;

    /* UI overlay. */
    mve::ByteImage::Ptr ui_image;
    ogl::Texture::Ptr gui_texture;
    ogl::VertexArray::Ptr gui_renderer;
    bool ui_needs_update;

    /* Frustum selector stuff. */
    bool rect_shift_pressed;
    int rect_start_x;
    int rect_start_y;
    int rect_current_x;
    int rect_current_y;

    /* Scene rendering settings. */
    QCheckBox draw_worldaxis_cb;
    QCheckBox draw_sfmpoints_cb;
    QCheckBox draw_camfrusta_cb;
    QCheckBox draw_curfrustum_cb;
    QCheckBox draw_mesh_lighting_cb;
    QColor clear_color;
    QCheckBox clear_color_cb;
    QSlider draw_frusta_size;

    /* Mesh rendering settings. */
    QCheckBox draw_wireframe_cb;
    QCheckBox draw_meshcolor_cb;

    /* Offscreen rendering settings. */
    QFileSelector offscreen_seqfile;
    QFileSelector offscreen_framedir;
    QPushButton offscreen_playbut;
    QSpinBox offscreen_width;
    QSpinBox offscreen_height;
    bool offscreen_working;
    QLineEdit offscreen_rephoto_source;
    QLineEdit offscreen_rephoto_color_dest;
    QLineEdit offscreen_rephoto_depth_dest;

    /* DM triangulate settings. */
    QComboBox dm_depthmap;
    QComboBox dm_colorimage;
    QDoubleSpinBox dm_depth_disc;
    QPushButton dm_triangulate_but;

private slots:
    void on_dm_triangulate (void);
    void select_colorimage (QString name);
    void apply_clear_color (void);
    void on_set_clear_color (void);
    void on_frusta_size_changed (void);
    void on_offscreen_snapshot (void);
    void on_offscreen_rephoto (void);
    void on_offscreen_rephoto (mve::View::Ptr view);
    void on_offscreen_rephoto_all (void);
    void on_offscreen_render_sequence (void);
    void on_offscreen_play_sequence (bool save = false);
    void on_offscreen_display_sequence (void);
    void on_recreate_sfm_renderer (void);

private:
    void create_frusta_renderer (float size);
    void create_current_frustum_renderer (void);
    void create_sfm_renderer (void);
    void update_ui (void);
    void screen_debug (float left, float right, float top, float bottom);

    void print_error (std::string const& error, std::string const& message);
    void print_info (std::string const& title, std::string const& message);
    void print_html (std::string const& title, std::string const& message);
    mve::ByteImage::Ptr get_offscreen_image (void);

protected:
    void init_impl (void);
    void paint_impl (void);
    void resize_impl (int old_width, int old_height);

    void mouse_event (ogl::MouseEvent const& event);
    void keyboard_event (ogl::KeyboardEvent const& event);

public:
    SceneContext (void);

    void reload_shaders (void);
    void load_file (std::string const& filename);
    void set_view (mve::View::Ptr view);
    void set_scene (mve::Scene::Ptr scene);
    char const* get_gui_name (void) const;
    void reset (void);
};

/* ---------------------------------------------------------------- */

inline char const*
SceneContext::get_gui_name (void) const
{
    return "Scene";
}

inline void
SceneContext::reset (void)
{
    this->scene.reset();
    this->view->set_view(mve::View::Ptr());
}

#endif /* UMVE_SCENECONTEXT_HEADER */
