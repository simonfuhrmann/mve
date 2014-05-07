/*
 * Exhaustive exact nearest neighbor search.
 * Written by Simon Fuhrmann.
 */

#ifndef SFM_NEAREST_NEIGHBOR_HEADER
#define SFM_NEAREST_NEIGHBOR_HEADER

#include "sfm/defines.h"

/* Whether to use SSE optimizations. */
#define ENABLE_SSE2 1
#define ENABLE_SSE3 1

SFM_NAMESPACE_BEGIN

/**
 * Nearest (and second nearest) neighbor search for normalized vectors.
 *
 * Finding the nearest neighbor for a query Q in a list of candidates Ci
 * boils down to finding the Ci with smallest distance ||Q - Ci||, or smallest
 * squared distance ||Q - Ci||^2 (which is cheaper to compute).
 *
 *   ||Q - Ci||^2 = ||Q||^2 + ||Ci||^2 - 2 * <Q | Ci>.
 *
 * If Q and Ci are expected to be normalized, ||Q - Ci||^2 = 2 - 2 * <Q | Ci>.
 * Thus, we want to quickly compute and find the largest inner product <Q, Ci>
 * corresponding to the smallest distance.
 *
 * A few IMPORTANT notes:
 *
 * - SSE2 accellerated dot product is implemented for:
 *   - short: vector dimension must be a factor of 8 (128 bit SSE2).
 *   - provide 16 byte aligned elements and query memory for SSE2.
 *
 * - Unaccellerated implementations:
 *   - float: No constraints.
 */
template <typename T>
class NearestNeighbor
{
public:
    /** Unlike the naming suggests, these are square distances. */
    struct Result
    {
        T dist_1st_best;
        T dist_2nd_best;
        int index_1st_best;
        int index_2nd_best;
    };

public:
    NearestNeighbor (void);
    /** For SfM, this is the descriptor memory block. */
    void set_elements (T const* elements, int num_elements);
    /** For SfM, this is the descriptor length. */
    void set_element_dimensions (int element_dimensions);
    /** Find the nearest neighbor of 'query'. */
    void find (T const* query, Result* result) const;

    int get_element_dimensions (void) const;

private:
    int dimensions;
    T const* elements;
    int num_elements;
};

/* ---------------------------------------------------------------- */

template <typename T>
inline
NearestNeighbor<T>::NearestNeighbor (void)
{
    this->dimensions = 64;
    this->elements = 0;
    this->num_elements = 0;
}

template <typename T>
inline void
NearestNeighbor<T>::set_elements (T const* elements, int num_elements)
{
    this->elements = elements;
    this->num_elements = num_elements;
}

template <typename T>
inline void
NearestNeighbor<T>::set_element_dimensions (int element_dimensions)
{
    this->dimensions = element_dimensions;
}

template <typename T>
inline int
NearestNeighbor<T>::get_element_dimensions (void) const
{
    return this->dimensions;
}

SFM_NAMESPACE_END

#endif  /* SFM_NEAREST_NEIGHBOR_HEADER */
