#include "ogl/render_tools.h"

#include "scene_addins/addin_axis_renderer.h"

AddinAxisRenderer::AddinAxisRenderer (void)
{
    this->render_cb = new QCheckBox("Draw world axis");
    this->render_cb->setChecked(true);
    this->connect(this->render_cb, SIGNAL(clicked()),
        this, SLOT(repaint()));
}

QWidget*
AddinAxisRenderer::get_sidebar_widget (void)
{
    return this->render_cb;
}

void
AddinAxisRenderer::paint_impl (void)
{
    if (this->render_cb->isChecked())
    {
        if (this->axis_renderer == NULL)
            this->axis_renderer = ogl::create_axis_renderer
                (this->state->wireframe_shader);

        this->state->wireframe_shader->bind();
        this->state->wireframe_shader->send_uniform("ccolor", math::Vec4f(0.0f));
        this->axis_renderer->draw();
    }
}
