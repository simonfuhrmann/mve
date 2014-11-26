#include "guihelpers.h"

#include "scene_addins/addin_frusta_base.h"
#include "scene_addins/addin_frusta_sfm_renderer.h"

AddinFrustaSfmRenderer::AddinFrustaSfmRenderer (void)
{
    this->render_frusta_cb = new QCheckBox("Draw camera frusta");
    this->render_frusta_cb->setChecked(true);

    this->frusta_size_slider = new QSlider();
    this->frusta_size_slider->setMinimum(1);
    this->frusta_size_slider->setMaximum(100);
    this->frusta_size_slider->setValue(10);
    this->frusta_size_slider->setOrientation(Qt::Horizontal);

    /* Create frusta rendering layout. */
    this->render_frusta_form = new QFormLayout();
    this->render_frusta_form->setVerticalSpacing(0);
    this->render_frusta_form->addRow(this->render_frusta_cb);
    this->render_frusta_form->addRow("Frusta Size:", this->frusta_size_slider);

    this->connect(this->frusta_size_slider, SIGNAL(valueChanged(int)),
        this, SLOT(reset_frusta_renderer()));
    this->connect(this->frusta_size_slider, SIGNAL(valueChanged(int)),
        this, SLOT(repaint()));
    this->connect(this->render_frusta_cb, SIGNAL(clicked()),
        this, SLOT(repaint()));
}

QWidget*
AddinFrustaSfmRenderer::get_sidebar_widget (void)
{
    return get_wrapper(this->render_frusta_form);
}

void
AddinFrustaSfmRenderer::set_cameras (std::vector<mve::CameraInfo> const& cameras)
{
    this->cameras = cameras;
    this->frusta_renderer.reset();
}

void
AddinFrustaSfmRenderer::paint_impl (void)
{
    if (this->render_frusta_cb->isChecked())
    {
        if (this->frusta_renderer == NULL)
            this->create_frusta_renderer();
        if (this->frusta_renderer != NULL)
            this->frusta_renderer->draw();
    }
}

void
AddinFrustaSfmRenderer::reset_frusta_renderer (void)
{
    this->frusta_renderer.reset();
}

void
AddinFrustaSfmRenderer::create_frusta_renderer (void)
{
    float size = this->frusta_size_slider->value() / 100.0f;
    mve::TriangleMesh::Ptr mesh = mve::TriangleMesh::create();
    for (std::size_t i = 0; i < this->cameras.size(); ++i)
    {
        if (this->cameras[i].flen == 0.0f)
            continue;
        add_camera_to_mesh(this->cameras[i], size, mesh);
    }

    this->frusta_renderer = ogl::MeshRenderer::create(mesh);
    this->frusta_renderer->set_shader(this->state->wireframe_shader);
    this->frusta_renderer->set_primitive(GL_LINES);
}
