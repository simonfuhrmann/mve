#include "guihelpers.h"

#include "scenemanager.h"
#include "scene_addins/addin_frusta_renderer.h"

AddinFrustaRenderer::AddinFrustaRenderer (void)
{
    this->last_view = NULL;
    this->render_frusta_cb = new QCheckBox("Draw camera frusta");
    this->render_viewdir_cb = new QCheckBox("Draw viewing direction");
    this->render_frusta_cb->setChecked(true);
    this->render_viewdir_cb->setChecked(true);

    this->frusta_size_slider = new QSlider();
    this->frusta_size_slider->setMinimum(1);
    this->frusta_size_slider->setMaximum(100);
    this->frusta_size_slider->setValue(10);
    this->frusta_size_slider->setOrientation(Qt::Horizontal);

    /* Create frusta rendering layout. */
    this->render_frusta_form = new QFormLayout();
    this->render_frusta_form->setVerticalSpacing(0);
    this->render_frusta_form->addRow(this->render_frusta_cb);
    this->render_frusta_form->addRow(this->render_viewdir_cb);
    this->render_frusta_form->addRow("Frusta Size:", this->frusta_size_slider);

    this->connect(&SceneManager::get(), SIGNAL(scene_bundle_changed()),
        this, SLOT(reset_frusta_renderer()));
    this->connect(&SceneManager::get(), SIGNAL(scene_selected(mve::Scene::Ptr)),
        this, SLOT(reset_frusta_renderer()));
    this->connect(&SceneManager::get(), SIGNAL(view_selected(mve::View::Ptr)),
        this, SLOT(reset_viewdir_renderer()));
    this->connect(this->frusta_size_slider, SIGNAL(valueChanged(int)),
        this, SLOT(reset_frusta_renderer()));
    this->connect(this->frusta_size_slider, SIGNAL(valueChanged(int)),
        this, SLOT(repaint()));
    this->connect(this->render_frusta_cb, SIGNAL(clicked()),
        this, SLOT(repaint()));
    this->connect(this->render_viewdir_cb, SIGNAL(clicked()),
        this, SLOT(repaint()));
}

QWidget*
AddinFrustaRenderer::get_sidebar_widget (void)
{
    return get_wrapper(this->render_frusta_form);
}

void
AddinFrustaRenderer::reset_frusta_renderer (void)
{
    this->frusta_renderer.reset();
}

void
AddinFrustaRenderer::reset_viewdir_renderer (void)
{
    this->viewdir_renderer.reset();
}

void
AddinFrustaRenderer::paint_impl (void)
{
    if (this->render_frusta_cb->isChecked())
    {
        if (this->frusta_renderer == NULL)
            this->create_frusta_renderer();
        if (this->frusta_renderer != NULL)
            this->frusta_renderer->draw();
    }

    if (this->render_viewdir_cb->isChecked())
    {
        if (this->viewdir_renderer == NULL)
            this->create_viewdir_renderer();
        if (this->viewdir_renderer != NULL)
            this->viewdir_renderer->draw();
    }
}

void
AddinFrustaRenderer::create_frusta_renderer (void)
{
    if (this->state->scene == NULL)
        return;

    /* Color at camera center (start) and at the four corners (end). */
    float size = this->frusta_size_slider->value() / 100.0f;
    math::Vec4f const frustum_start_color(0.5f, 0.5f, 0.5f, 1.0f);
    math::Vec4f const frustum_end_color(0.5f, 0.5f, 0.5f, 1.0f);

    mve::TriangleMesh::Ptr mesh = mve::TriangleMesh::create();
    mve::TriangleMesh::VertexList& verts = mesh->get_vertices();
    mve::TriangleMesh::ColorList& colors = mesh->get_vertex_colors();
    mve::TriangleMesh::FaceList& faces = mesh->get_faces();

    mve::Scene::ViewList const& views(this->state->scene->get_views());
    for (std::size_t i = 0; i < views.size(); ++i)
    {
        if (views[i].get() == NULL)
            continue;

        mve::CameraInfo const& cam = views[i]->get_camera();
        if (cam.flen == 0.0f)
            continue;

        /* Get camera position and transformations. */
        math::Vec3f campos;
        math::Matrix4f ctw;
        cam.fill_camera_pos(*campos);
        cam.fill_cam_to_world(*ctw);

        math::Vec3f cam_x(1.0f, 0.0f, 0.0f);
        math::Vec3f cam_y(0.0f, 1.0f, 0.0f);
        math::Vec3f cam_z(0.0f, 0.0f, 1.0f);
        cam_x = ctw.mult(cam_x, 0.0f);
        cam_y = ctw.mult(cam_y, 0.0f);
        cam_z = ctw.mult(cam_z, 0.0f);

        std::size_t idx(verts.size());

        /* Vertices, colors and faces for the frustum. */
        verts.push_back(campos);
        colors.push_back(frustum_start_color);
        for (int j = 0; j < 4; ++j)
        {
            math::Vec3f corner = campos + size * cam_z
                + cam_x * size / (2.0f * cam.flen) * (j & 1 ? -1.0f : 1.0f)
                + cam_y * size / (2.0f * cam.flen) * (j & 2 ? -1.0f : 1.0f);
            verts.push_back(corner);
            colors.push_back(frustum_end_color);
            faces.push_back(idx + 0); faces.push_back(idx + 1 + j);
        }
        faces.push_back(idx + 1); faces.push_back(idx + 2);
        faces.push_back(idx + 2); faces.push_back(idx + 4);
        faces.push_back(idx + 4); faces.push_back(idx + 3);
        faces.push_back(idx + 3); faces.push_back(idx + 1);

        /* Vertices, colors and faces for the local coordinate system. */
        verts.push_back(campos);
        verts.push_back(campos + (size * 0.5f) * cam_x);
        verts.push_back(campos);
        verts.push_back(campos + (size * 0.5f) * cam_y);
        verts.push_back(campos);
        verts.push_back(campos + (size * 0.5f) * cam_z);
        colors.push_back(math::Vec4f(1.0f, 0.0f, 0.0f, 1.0f));
        colors.push_back(math::Vec4f(1.0f, 0.0f, 0.0f, 1.0f));
        colors.push_back(math::Vec4f(0.0f, 1.0f, 0.0f, 1.0f));
        colors.push_back(math::Vec4f(0.0f, 1.0f, 0.0f, 1.0f));
        colors.push_back(math::Vec4f(0.0f, 0.0f, 1.0f, 1.0f));
        colors.push_back(math::Vec4f(0.0f, 0.0f, 1.0f, 1.0f));
        faces.push_back(idx + 5); faces.push_back(idx + 6);
        faces.push_back(idx + 7); faces.push_back(idx + 8);
        faces.push_back(idx + 9); faces.push_back(idx + 10);
    }

    this->frusta_renderer = ogl::MeshRenderer::create(mesh);
    this->frusta_renderer->set_shader(this->state->wireframe_shader);
    this->frusta_renderer->set_primitive(GL_LINES);
}

void
AddinFrustaRenderer::create_viewdir_renderer (void)
{
    if (this->state->view == NULL)
        return;

    math::Vec3f campos, viewdir;
    mve::CameraInfo const& cam(this->state->view->get_camera());
    cam.fill_camera_pos(*campos);
    cam.fill_viewing_direction(*viewdir);

    mve::TriangleMesh::Ptr mesh = mve::TriangleMesh::create();
    mve::TriangleMesh::VertexList& verts = mesh->get_vertices();
    mve::TriangleMesh::ColorList& colors = mesh->get_vertex_colors();
    verts.push_back(campos);
    verts.push_back(campos + viewdir * 100.0f);
    colors.push_back(math::Vec4f(1.0f, 1.0f, 0.0f, 1.0f));
    colors.push_back(math::Vec4f(1.0f, 1.0f, 0.0f, 1.0f));

    this->viewdir_renderer = ogl::MeshRenderer::create(mesh);
    this->viewdir_renderer->set_shader(this->state->wireframe_shader);
    this->viewdir_renderer->set_primitive(GL_LINES);
    this->last_view = this->state->view;
}
