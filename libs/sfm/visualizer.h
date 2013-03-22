#ifndef SFM_MATCHINGVISUALIZER_HEADER
#define SFM_MATCHINGVISUALIZER_HEADER

#include <utility>
#include <vector>

#include "mve/image.h"
#include "sfm/defines.h"
#include "sfm/correspondence.h"

SFM_NAMESPACE_BEGIN

class Visualizer
{
public:
    struct Keypoint
    {
        float x;
        float y;
        float radius;
        float orientation;
    };

    enum KeypointStyle
    {
        RADIUS_BOX_ORIENTATION,
        RADIUS_CIRCLE_ORIENTATION,
        SMALL_CIRCLE_STATIC,
        SMALL_DOT_STATIC
    };

public:
    /**
     * Draws a single feature on the image.
     */
    static void draw_keypoint (mve::ByteImage& image,
        Keypoint const& keypoint, KeypointStyle style, uint8_t const* color);

    /**
     * Draws a list of features on a grayscale version of the image.
     */
    static mve::ByteImage::Ptr draw_keypoints (mve::ByteImage::ConstPtr image,
        std::vector<Keypoint> const& matches, KeypointStyle style);

    /**
     * Places images next to each other and draws a list of matches.
     */
    static mve::ByteImage::Ptr draw_matches (mve::ByteImage::ConstPtr image1,
        mve::ByteImage::ConstPtr image2, Correspondences const& matches);
};

SFM_NAMESPACE_END

#endif /* SFM_MATCHINGVISUALIZER_HEADER */
