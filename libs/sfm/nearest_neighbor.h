/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef SFM_NEAREST_NEIGHBOR_HEADER
#define SFM_NEAREST_NEIGHBOR_HEADER

#include "sfm/defines.h"

#define ENABLE_SSE2_NN_SEARCH 1
#define ENABLE_SSE3_NN_SEARCH 1

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
 * If Q and Ci are normalized, ||Q - Ci||^2 = 2 - 2 * <Q | Ci>.
 * Thus, we want to quickly compute and find the largest inner product <Q, Ci>
 * corresponding to the smallest distance.
 *
 * Notes: For SSE accellerated dot products, vector dimension must be a factor
 * of 8 (i.e. 128 bit registers for SSE). Query and elements must be 16 byte
 * aligned for efficient memory access.
 *
 * The following types are supported:
 *   - signed short using SSE2
 *     value range -127 to 127, normalized to 127, max distance 32258
 *   - unsigend short using SSE2
 *     value range 0 to 255, normalized to 255, max distance 65534
 *   - float using SSE3
 *     any value range, normalized to 1, any distance possible
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
    void set_elements (T const* elements);
    /** For SfM, this is the descriptor length. */
    void set_element_dimensions (int element_dimensions);
    /** For SfM, this is the number of descriptors. */
    void set_num_elements (int num_elements);
    /** Find the nearest neighbor of 'query'. */
    void find (T const* query, Result* result) const;

    int get_element_dimensions (void) const;

private:
    int dimensions;
    int num_elements;
    T const* elements;
};

/* ---------------------------------------------------------------- */

template <typename T>
inline
NearestNeighbor<T>::NearestNeighbor (void)
    : dimensions(64)
    , num_elements(0)
    , elements(nullptr)
{
}

template <typename T>
inline void
NearestNeighbor<T>::set_elements (T const* elements)
{
    this->elements = elements;
}

template <typename T>
inline void
NearestNeighbor<T>::set_element_dimensions (int element_dimensions)
{
    this->dimensions = element_dimensions;
}

template <typename T>
inline void
NearestNeighbor<T>::set_num_elements (int num_elements)
{
    this->num_elements = num_elements;
}

template <typename T>
inline int
NearestNeighbor<T>::get_element_dimensions (void) const
{
    return this->dimensions;
}

SFM_NAMESPACE_END

#endif  /* SFM_NEAREST_NEIGHBOR_HEADER */
