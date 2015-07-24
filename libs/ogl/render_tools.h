/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef OGL_RENDERTOOLS_HEADER
#define OGL_RENDERTOOLS_HEADER

#include "ogl/defines.h"
#include "ogl/vertex_array.h"
#include "ogl/shader_program.h"

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
