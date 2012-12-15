/*
 * Exhaustive exact nearest neighbor search.
 * Written by Simon Fuhrmann.
 */

#ifndef SFM_NEARESTNEIGHBOR_HEADER
#define SFM_NEARESTNEIGHBOR_HEADER

#include "defines.h"

/* Whether to use SSE optimizations. */
#define ENABLE_SSE2 1

SFM_NAMESPACE_BEGIN

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
 *
 * TODO: SSE does only work with certian element sizes... explain.
 */
template <typename T>
class NearestNeighbor
{
public:
    struct Result
    {
        T dist_1st_best;
        T dist_2nd_best;
        int index_1st_best;
        int index_2nd_best;
    };

public:
    NearestNeighbor (void);
    /** For SfM, this is the descriptor memory blcok. */
    void set_elements (T const* elements, int num_elements);
    /** For SfM, this is the descriptor length. */
    void set_element_dimensions (int element_dimensions);
    /** Find the nearest neighbor of 'query'. */
    void find (T const* query, Result* result) const;

private:
    int dimensions;
    T const* elements;
    int num_elements;
};

/* ---------------------------------------------------------------- */

template <typename T>
NearestNeighbor<T>::NearestNeighbor (void)
{
    this->dimensions = 64;
    this->elements = 0;
    this->num_elements = 0;
}

template <typename T>
void
NearestNeighbor<T>::set_elements (T const* elements, int num_elements)
{
    this->elements = elements;
    this->num_elements = num_elements;
}

template <typename T>
void
NearestNeighbor<T>::set_element_dimensions (int element_dimensions)
{
    this->dimensions = element_dimensions;
}

SFM_NAMESPACE_END

#endif  /* SFM_NEARESTNEIGHBOR_HEADER */
