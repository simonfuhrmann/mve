#include <limits>
#include <iostream>
#include <stdexcept>

#include "math/algo.h"
#include "seamcarving.h"

MVE_NAMESPACE_BEGIN

void
SeamCarving::set_image (mve::ByteImage::ConstPtr image)
{
    if (!image.get())
        throw std::invalid_argument("NULL image given");

    this->img = *image;
    this->cw = this->img.width();
    this->ch = this->img.height();
    this->cc = this->img.channels();
    this->build_cost_image();
}

/* ---------------------------------------------------------------- */

ByteImage::Ptr
SeamCarving::exec (void)
{
    if (!this->img.valid() || !this->cost.valid())
        throw std::runtime_error("No image set");

    if (!this->dimx || !this->dimy)
        throw std::runtime_error("No target dimensions set");

    /* Iteratively remove seams. */
    while (this->cw > this->dimx || this->ch > this->dimy)
        this->remove_optimal_seam();

    mve::ByteImage::Ptr out(mve::ByteImage::create(this->cw, this->ch, this->cc));
    for (std::size_t y = 0; y < this->ch; ++y)
    {
        uint8_t* outrow = &out->at(0, y, 0);
        uint8_t* inrow = &this->img(0, y, 0);
        std::copy(inrow, inrow + this->cw * this->cc, outrow);
    }

    return out;
}

/* ---------------------------------------------------------------- */

void
SeamCarving::remove_optimal_seam (void)
{
    /* Find best vertical seam and remember. */
    int vseam_energy = std::numeric_limits<int>::max();
    Seam vseam;
    if (this->cw > this->dimx)
    {
        vseam_energy = this->find_optimal_seam(vseam, this->cost, this->cw, this->ch);
    }

    /* Find best horizontal seam and remember. */
    int hseam_energy = std::numeric_limits<int>::max();
    Seam hseam(this->cw);
    if (this->ch > this->dimy)
    {
        IntImage tmp_costs(this->ch, this->cw, 1);
        for (std::size_t y = 0; y < this->ch; ++y)
            for (std::size_t x = 0; x < this->cw; ++x)
                tmp_costs(y, x, 0) = this->cost(x, y, 0);
        hseam_energy = this->find_optimal_seam(hseam, tmp_costs, this->ch, this->cw);
    }

    /* Remove best seam, either vertical or horizontal. */
    if (hseam_energy < vseam_energy)
        this->remove_hseam(hseam);
    else
        this->remove_vseam(vseam);
}

/* ---------------------------------------------------------------- */

int
SeamCarving::find_optimal_seam (Seam& seam, IntImage const& cimg,
    std::size_t ciw, std::size_t cih)
{
    seam.clear();

    mve::IntImage ccost(ciw, cih, 1);
    mve::CharImage path(ciw, cih, 1);

    /* Copy first row of the energy image. */
    ccost.fill(std::numeric_limits<int>::max());
    std::copy(&cimg(0), &cimg(0) + ciw, &ccost(0));
    std::fill(&path(0), &path(0) + ciw, 0);

    /* Now cumulate energy from top to bottom and remember path. */
    for (std::size_t y = 1; y < cih; ++y)
        for (std::size_t x = 0; x < ciw; ++x)
        {
            int* ccost_pixel = &ccost.at(x, y, 0);
            int const* cost_pixel = &cimg(x, y, 0);
            char* pathv = &path(x, y, 0);

            int min_dx = (x > 0 ? -1 : 0);
            int max_dx = (x + 1 < ciw ? 1 : 0);
            for (int dx = min_dx; dx <= max_dx; ++dx)
            {
                std::size_t px = x + dx;
                std::size_t py = y - 1;
                int cost_sum = ccost(px, py, 0) + *cost_pixel;
                /* Remember smallest cost and path. */
                if (*ccost_pixel > cost_sum)
                {
                    *ccost_pixel = cost_sum;
                    *pathv = dx;
                }
            }
        }

    /* Find smallest cumulative cost in bottom row and backtrace seam. */
    int min_energy = std::numeric_limits<int>::max();
    std::size_t min_x = 0;
    {
        int* ccost_row = &ccost(0, cih - 1, 0);
        for (std::size_t x = 0; x < ciw; ++x)
        {
            if (ccost_row[x] < min_energy)
            {
                min_energy = ccost_row[x];
                min_x = x;
            }
        }
    }

    /* Backtrace and remember seam. */
    seam.resize(cih, 0);
    for (std::size_t y = cih - 1; y < cih; --y)
    {
        seam[y] = min_x;
        min_x += path(min_x, y, 0);
    }

    return min_energy;
}

/* ---------------------------------------------------------------- */

void
SeamCarving::remove_vseam (Seam const& seam)
{
    for (std::size_t y = 0; y < this->ch; ++y)
        for (std::size_t x = seam[y]; x < this->cw - 1; ++x)
        {
            /* Remove pixel in image. */
            {
                uint8_t* src = &this->img(x + 1, y, 0);
                uint8_t* dst = &this->img(x, y, 0);
                std::copy(src, src + this->cc, dst);
            }

            /* Remove pixel in cost image. */
            this->cost(x, y, 0) = this->cost(x + 1, y, 0);
        }

    if (this->rebuild)
        this->build_cost_image();

    this->cw -= 1;
    std::cout << "Removed v-seam, dimension: " << this->cw
         << "x" << this->ch << "." << std::endl;
}

/* ---------------------------------------------------------------- */

void
SeamCarving::remove_hseam (Seam const& seam)
{
    for (std::size_t x = 0; x < this->cw; ++x)
        for (std::size_t y = seam[x]; y < this->ch - 1; ++y)
        {
            /* Remove pixel in image. */
            {
                uint8_t* src = &this->img(x, y + 1, 0);
                uint8_t* dst = &this->img(x, y, 0);
                std::copy(src, src + this->cc, dst);
            }

            /* Remove pixel in cost image. */
             this->cost(x, y, 0) = this->cost(x, y + 1, 0);
        }

    if (this->rebuild)
        this->build_cost_image();

    this->ch -= 1;
    std::cout << "Removed h-seam, dimension: " << this->cw
         << "x" << this->ch << "." << std::endl;
}

/* ---------------------------------------------------------------- */

void
SeamCarving::build_cost_image (void)
{
    std::size_t iw(this->img.width());
    std::size_t ih(this->img.height());
    this->cost.allocate(iw, ih, 1);

    std::size_t idx = 0;
    for (std::size_t y = 0; y < ih; ++y)
        for (std::size_t x = 0; x < iw; ++x, ++idx)
            this->cost(idx) = this->pixel_cost(x, y);
}

/* ---------------------------------------------------------------- */

int
SeamCarving::pixel_cost (std::size_t x, std::size_t y)
{
    std::size_t const one(1);
    std::size_t cx = math::algo::clamp(x, one, this->cw - 2);
    std::size_t cy = math::algo::clamp(y, one, this->ch - 2);

    int dx(0), dy(0);
    for (std::size_t c = 0; c < this->cc; ++c)
    {
        int local_dx = int(this->img(cx - 1, cy, c)) - this->img(cx + 1, cy, c);
        int local_dy = int(this->img(cx, cy - 1, c)) - this->img(cx, cy + 1, c);
        dx = std::max(dx, std::abs(local_dx));
        dy = std::max(dy, std::abs(local_dy));
    }

    return dx + dy;
}

/* ---------------------------------------------------------------- */

ByteImage::Ptr
SeamCarving::get_cost_image (void) const
{
    mve::ByteImage::Ptr ret(mve::ByteImage::create(this->cw, this->ch, 1));
    std::size_t amount = ret->get_value_amount();
    for (std::size_t i = 0; i < amount; ++i)
        ret->at(i) = (uint8_t)math::algo::clamp
            (this->cost(i % this->cw, i / this->cw, 0), 0, 255);
    return ret;
}

/* ---------------------------------------------------------------- */

MVE_NAMESPACE_END
