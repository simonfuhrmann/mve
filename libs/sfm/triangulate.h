#ifndef SFM_TRIANGULATE_HEADER
#define SFM_TRIANGULATE_HEADER

#include <vector>

#include "math/vector.h"

#include "defines.h"
#include "correspondence.h"
#include "pose.h"

SFM_NAMESPACE_BEGIN

/**
 * Given an image correspondence in two views and the corresponding pose,
 * this function computes the 3D point coordinate using the DLT algorithm.
 */
math::Vector<double, 3>
triangulate_match (Correspondence const& match,
    CameraPose const& pose1, CameraPose const& pose2);

/**
 * Given a two-view pose configuration and a correspondence, this function
 * returns true if the triangulated point is in front of both cameras.
 */
bool
is_consistent_pose (Correspondence const& match,
    CameraPose const& pose1, CameraPose const& pose2);

SFM_NAMESPACE_END

#endif // SFM_TRIANGULATE_HEADER
