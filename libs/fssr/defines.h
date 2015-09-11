/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef FSSR_DEFINES_HEADER
#define FSSR_DEFINES_HEADER

#define FSSR_NAMESPACE_BEGIN namespace fssr {
#define FSSR_NAMESPACE_END }

/* Use new weighting function with continuous derivative. */
#define FSSR_NEW_WEIGHT_FUNCTION 1

/* Use derivatives for more precise isovertex interpolation. */
#define FSSR_USE_DERIVATIVES 1

/*
 * Using the new weighting function is strongly recommended when using
 * derivatives, as the old weighting function derivative is discontinuous.
 */
#if FSSR_USE_DERIVATIVES && !FSSR_NEW_WEIGHT_FUNCTION
#   error "FSSR_USE_DERIVATIVES requires FSSR_NEW_WEIGHT_FUNCTION"
#endif

#endif /* FSSR_DEFINES_HEADER */
