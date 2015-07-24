/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include "math/vector.h"
#include "math/matrix.h"

#include "scene_addins/addin_frusta_base.h"

void
add_camera_to_mesh (mve::CameraInfo const& camera,
    float size, mve::TriangleMesh::Ptr mesh)
{
    math::Vec4f const frustum_start_color(0.5f, 0.5f, 0.5f, 1.0f);
    math::Vec4f const frustum_end_color(0.5f, 0.5f, 0.5f, 1.0f);

    mve::TriangleMesh::VertexList& verts = mesh->get_vertices();
    mve::TriangleMesh::ColorList& colors = mesh->get_vertex_colors();
    mve::TriangleMesh::FaceList& faces = mesh->get_faces();

    /* Camera local coordinate system. */
    math::Matrix4f ctw;
    camera.fill_cam_to_world(*ctw);
    math::Vec3f cam_x = ctw.mult(math::Vec3f(1.0f, 0.0f, 0.0f), 0.0f);
    math::Vec3f cam_y = ctw.mult(math::Vec3f(0.0f, 1.0f, 0.0f), 0.0f);
    math::Vec3f cam_z = ctw.mult(math::Vec3f(0.0f, 0.0f, 1.0f), 0.0f);
    math::Vec3f campos = ctw.mult(math::Vec3f(0.0f), 1.0f);

    /* Vertices, colors and faces for the frustum. */
    std::size_t idx = verts.size();
    verts.push_back(campos);
    colors.push_back(frustum_start_color);
    for (int j = 0; j < 4; ++j)
    {
        math::Vec3f corner = campos + size * cam_z
            + cam_x * size / (2.0f * camera.flen) * (j & 1 ? -1.0f : 1.0f)
            + cam_y * size / (2.0f * camera.flen) * (j & 2 ? -1.0f : 1.0f);
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
