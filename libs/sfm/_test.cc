#include <iostream>

#include "util/timer.h"
#include "mve/image.h"
#include "mve/imagefile.h"

#include "surf.h"

#include <emmintrin.h>

template <typename T, uintptr_t MODULO = 16>
struct AlignedMemory
{
    AlignedMemory (void) : raw(0), aligned(0) {}
    AlignedMemory (std::size_t size) { this->allocate(size); }
    ~AlignedMemory (void) { this->deallocate(); }
    void allocate (std::size_t size)
    {
        this->raw = new unsigned char[(size + MODULO - 1) * sizeof(T)];
        uintptr_t tmp = reinterpret_cast<uintptr_t>(this->raw);
        this->aligned = (tmp & ~(MODULO - 1)) == tmp
            ? reinterpret_cast<T*>(tmp)
            : reinterpret_cast<T*>((tmp + MODULO) & ~(MODULO - 1));
    }
    void deallocate (void)
    {
        delete this->raw;
        this->raw = 0;
        this->aligned = 0;
    }

    unsigned char* raw;
    T* aligned;
};

/*
 *
X86/MMX/SSE1/SSE2	-mfpmath=sse -mmmx -msse -msse2
ARM Neon            -mfpu=neon -mfloat-abi=softfp
Freescale Altivec	-maltivec -mabi=altivec

arm_neon.h      ARM Neon types & intrinsics
altivec.h       Freescale Altivec types & intrinsics
mmintrin.h      X86 MMX
xmmintrin.h     X86 SSE1
emmintrin.h     X86 SSE2

MMX: __m64 64 bits of integers broken down as eight 8-bit integers, four 16-bit shorts or two 32-bit integers.
SSE1: __m128 128 bits: four single precision floats.
SSE2: __m128i 128 bits of any size packed integers, __m128d 128 bits: two doubles.

__MMX__ -- X86 MMX
__SSE__ -- X86 SSE
__SSE2__ -- X86 SSE2
__VEC__ -- altivec functions
__ARM_NEON__ -- neon functions
*/

// FIXME: Zero descriptors give zero distance
//

/**
 * Finding the nearest neighbor for a query Q in a list of candidates Ci
 * boils down to finding the Ci with smallest ||Q - Ci||, or finding the
 * smallest squared distance ||Q - Ci||^2 which is cheaper to compute.
 *
 *   ||Q - Ci||^2 = ||Q||^2 + ||Ci||^2 - 2 * Q * Ci.
 *
 * Since Q and Ci are normalized, ||Q - Ci||^2 = 2 - 2 * Q * Ci. In high
 * dimensional vector spaces, we want to quickly compute and find the largest
 * inner product <Q, Ci> corresponding to the smallest distance. Here, SSE2
 * parallel integer instructions are used to accellerate the search.
 */
void
sse2_matching (short* query, short* descr, int num_descr)
{
    /* Matching result. */
    int best_1st_index = 0;
    int best_2nd_index = 0;
    int best_1st_dist = std::numeric_limits<int>::max();
    int best_2nd_dist = std::numeric_limits<int>::max();

    /* Descriptor matching. */
    short result[8];
    __m128i* descr_ptr = reinterpret_cast<__m128i*>(descr);
    for (int descr_iter = 0; descr_iter < num_descr; ++descr_iter)
    {
        /* Compute dot product between query and candidate. */
        __m128i* query_ptr = reinterpret_cast<__m128i*>(query);
        __m128i reg_result = _mm_set1_epi16(0);
        for (int i = 0; i < 8; ++i, ++query_ptr, ++descr_ptr)
        {
            __m128i reg_query = _mm_load_si128(query_ptr);
            __m128i reg_subject = _mm_load_si128(descr_ptr);
            reg_result = _mm_add_epi16(reg_result,
                _mm_mullo_epi16(reg_query, reg_subject));
        }
        _mm_store_si128(reinterpret_cast<__m128i*>(result), reg_result);
        int inner_product = result[0] + result[1] + result[2] + result[3]
            + result[4] + result[5] + result[6] + result[7];

        /* Check if new largest inner product has been found. */
        if (inner_product > best_2nd_dist)
        {
            if (inner_product > best_1st_dist)
            {
                best_2nd_index = best_1st_index;
                best_2nd_dist = best_1st_dist;
                best_1st_index = descr_iter;
                best_1st_dist = inner_product;
            }
            else
            {
                best_2nd_index = descr_iter;
                best_2nd_dist = inner_product;
            }
        }
    }

    /*
     * Compute actual distances.
     * The distance with signed 'char' vectors is: 2 * 127^2 - 2 * <Q, Ci>.
     */
    best_1st_dist = 2 * 127 * 127 - 2 * best_1st_dist;
    best_2nd_dist = 2 * 127 * 127 - 2 * best_2nd_dist;
}

void
dumb_matching (short* query, short* descr, int num_descr)
{
    /* Matching result. */
    int best_1st_index = 0;
    int best_2nd_index = 0;
    int best_1st_dist = std::numeric_limits<int>::max();
    int best_2nd_dist = std::numeric_limits<int>::max();

    /* Descriptor matching. */
    short* descr_ptr = descr;
    for (int i = 0; i < num_descr; ++i)
    {
        int inner_product = 0;
        for (int i = 0; i < 64; ++i, ++descr_ptr)
            inner_product += query[i] * *descr_ptr;

        /* Check if new largest inner product has been found. */
        if (inner_product > best_2nd_dist)
        {
            if (inner_product > best_1st_dist)
            {
                best_2nd_index = best_1st_index;
                best_2nd_dist = best_1st_dist;
                best_1st_index = i;
                best_1st_dist = inner_product;
            }
            else
            {
                best_2nd_index = i;
                best_2nd_dist = inner_product;
            }
        }
    }

    /*
     * Compute actual distances.
     * The distance with signed 'char' vectors is: 2 * 127^2 - 2 * <Q, Ci>.
     */
    best_1st_dist = 2 * 127 * 127 - 2 * best_1st_dist;
    best_2nd_dist = 2 * 127 * 127 - 2 * best_2nd_dist;
}

int main (void)
{
#ifdef __SSE2__
    std::cout << "SSE2 is enabled!" << std::endl;
#endif

    int const num_descr = 1024000;
    AlignedMemory<short> mem(64 * num_descr);
    AlignedMemory<short> query(64);

    for (int i = 0; i < 11; ++i)
    {
        mem.aligned[i] = i;
        query.aligned[i] = i;
    }

    util::WallTimer timer;
    sse2_matching(query.aligned, mem.aligned, num_descr);
    std::cout << "took " << timer.get_elapsed() << "ms." << std::endl;

    timer.reset();
    dumb_matching(query.aligned, mem.aligned, num_descr);
    std::cout << "took " << timer.get_elapsed() << "ms." << std::endl;

#if 0
    std::vector<short> vec1(64);
    std::vector<short> vec2(64);
    for (std::size_t i = 0; i < vec1.size(); ++i)
    {
        vec1[i] = (i * 5) % 255 - 128;
        vec2[i] = (i * 3) % 255 - 128;
    }

    util::WallTimer timer;
    int result = 0;
    for (int i = 0; i < 1000000; ++i)
        result = inner_product(vec1, vec2);
    std::cout << "result is " << result << std::endl;
    std::cout << "took " << timer.get_elapsed() << "ms." << std::endl;
#endif

#if 0
    mve::ByteImage::Ptr image;
    try
    {
        image = mve::image::load_file("/tmp/image.jpg");
    }
    catch (std::exception& e)
    {
        std::cout << "Exception: " << e.what() << std::endl;
    }

    sfm::Surf surf;
    surf.set_image(image);
    surf.set_contrast_threshold(1000.0f);
    surf.process();

    surf.draw_features(image);
    mve::image::save_file(image, "/tmp/out.png");
#endif
    return 0;
}
