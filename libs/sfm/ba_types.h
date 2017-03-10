#ifndef SFM_BA_TYPES_HEADER
#define SFM_BA_TYPES_HEADER

#include <algorithm>

#include "sfm/defines.h"

SFM_NAMESPACE_BEGIN
SFM_BA_NAMESPACE_BEGIN

/** Camera representation for bundle adjustment. */
struct Camera
{
    Camera (void);

    double focal_length = 0.0;
    double distortion[2];
    double translation[3];
    double rotation[9];
    bool is_constant = false;
};

/** 3D point representation for bundle adjustment. */
struct Point3D
{
    double pos[3];
    bool is_constant = false;
};

/** Observation of a 3D point for a camera. */
struct Observation
{
    double pos[2];
    int camera_id;
    int point_id;
};

/* ------------------------ Implementation ------------------------ */

inline
Camera::Camera (void)
{
    std::fill(this->distortion, this->distortion + 2, 0.0);
    std::fill(this->translation, this->translation + 3, 0.0);
    std::fill(this->rotation, this->rotation + 9, 0.0);
}

SFM_BA_NAMESPACE_END
SFM_NAMESPACE_END

#endif /* SFM_BA_TYPES_HEADER */

