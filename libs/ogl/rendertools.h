/*
 * Tools for rendering.
 * Written by Simon Fuhrmann.
 */

#ifndef OGL_RENDERTOOLS_HEADER
#define OGL_RENDERTOOLS_HEADER

#include "ogl/defines.h"
#include "ogl/vertexarray.h"
#include "ogl/shaderprogram.h"

OGL_NAMESPACE_BEGIN

/**
 * Generates a vertex array for visualizing the three world coordinate axis.
 * You need to specify your own shader, where you can also apply additional
 * transformations, for example to visualize local coordinate system.
 */
VertexArray::Ptr
create_axis_renderer (ShaderProgram::Ptr shader);

/**
 * Generates a full screen quad renderer in OpenGL unit coordinates.
 * The quad vertices have coordiantes (+-1, +-1, 0) with
 * normals (0, 0, 1) and texture coordiantes (0/1, 0/1).
 */
VertexArray::Ptr
create_fullscreen_quad (ShaderProgram::Ptr shader);

OGL_NAMESPACE_END

#endif /* OGL_RENDERTOOLS_HEADER */
