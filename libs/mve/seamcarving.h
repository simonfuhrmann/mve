/*
 * Seam carving for content-aware image targeting.
 * Written by Simon Fuhrmann.
 */

#ifndef MVE_SEAMCARVING_HEADER
#define MVE_SEAMCARVING_HEADER

#include <vector>

#include "defines.h"
#include "image.h"

MVE_NAMESPACE_BEGIN

/**
 * A seam carving implementation for content-aware image resizing.
 * This technique is from the paper:
 *
 *    "Seam Carving for Content-Aware Image Resizing"
 *    Shai Avidan and Ariel Shamir
 *
 * The technique takes an input image and a desired new size.
 * An gradient image is used as cost image to remove seams with
 * minimal cost. Note that only shrinking is supported by now.
 */
class SeamCarving
{
private:
    typedef std::vector<int> Seam;

private:
    mve::ByteImage img; ///< Image to be modified
    mve::IntImage cost; ///< Cost function image
    std::size_t dimx; ///< Target x-dimension
    std::size_t dimy; ///< Target y-dimension

    std::size_t cw; ///< Current width
    std::size_t ch; ///< Current height
    std::size_t cc; ///< Channels

    bool rebuild; ///< Rebuild cost image after removal?

private:
    void remove_vseam (Seam const& seam);
    void remove_hseam (Seam const& seam);
    void remove_optimal_seam (void);
    int find_optimal_seam (Seam& seam, IntImage const& cimg,
        std::size_t ciw, std::size_t cih);

    void build_cost_image (void);
    int pixel_cost (std::size_t x, std::size_t y);

public:
    SeamCarving (void);

    /** Sets subject image. */
    void set_image (mve::ByteImage::ConstPtr image);
    /** Sets target dimensions. */
    void set_dimension (std::size_t width, std::size_t height);
    /** Sets whether cost image is rebuild are seam removal. */
    void set_rebuild_costs (bool rebuild);

    /** Returns the energy image. */
    ByteImage::Ptr get_cost_image (void) const;

    /** Executes algorithm and provides seam carved image. */
    mve::ByteImage::Ptr exec (void);
};

/* ---------------------------------------------------------------- */

inline
SeamCarving::SeamCarving (void)
    : dimx(0), dimy(0), rebuild(false)
{
}

inline void
SeamCarving::set_dimension (std::size_t width, std::size_t height)
{
    this->dimx = width;
    this->dimy = height;
}

inline void
SeamCarving::set_rebuild_costs (bool rebuild)
{
    this->rebuild = rebuild;
}

MVE_NAMESPACE_END

#endif /* MVE_SEAMCARVING_HEADER */
