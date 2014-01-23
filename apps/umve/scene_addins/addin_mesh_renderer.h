#ifndef UMVE_ADDIN_MESHES_RENDERER_HEADER
#define UMVE_ADDIN_MESHES_RENDERER_HEADER

#include <string>

#include <QWidget>
#include <QCheckBox>
#include <QBoxLayout>

#include "mve/mesh.h"

#include "scene_addins/mesh_list.h"
#include "scene_addins/addin_base.h"

class AddinMeshesRenderer : public AddinBase
{
    Q_OBJECT

public:
    AddinMeshesRenderer (void);
    QWidget* get_sidebar_widget (void);

    void add_mesh (std::string const& name,
        mve::TriangleMesh::Ptr mesh,
        std::string const& filename = "");

    void load_mesh (std::string const& filename);

protected:
    void paint_impl (void);

private:
    QVBoxLayout* render_meshes_box;
    QCheckBox* render_lighting_cb;
    QCheckBox* render_wireframe_cb;
    QCheckBox* render_color_cb;
    QMeshList* mesh_list;
};

#endif /* UMVE_ADDIN_MESHES_RENDERER_HEADER */
