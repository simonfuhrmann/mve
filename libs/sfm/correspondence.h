#ifndef SFM_CORRESPONDENCE_HEADER
#define SFM_CORRESPONDENCE_HEADER

#include <vector>

#include "defines.h"

SFM_NAMESPACE_BEGIN

struct Correspondence;
typedef std::vector<Correspondence> Correspondences;

/**
 * Two image coordinates which correspond to each other in terms of observing
 * the same point in the scene.
 */
struct Correspondence
{
    double p1[2];
    double p2[2];
};

SFM_NAMESPACE_END

#endif  // SFM_CORRESPONDENCE_HEADER
