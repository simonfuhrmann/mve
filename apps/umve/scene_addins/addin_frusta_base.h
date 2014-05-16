#ifndef UMVE_ADDIN_FRUSTA_BASE_HEADER
#define UMVE_ADDIN_FRUSTA_BASE_HEADER

#include "mve/camera.h"
#include "mve/mesh.h"

void add_camera_to_mesh (mve::CameraInfo const& camera,
    float size, mve::TriangleMesh::Ptr mesh);

#endif /* UMVE_ADDIN_FRUSTA_BASE_HEADER */
