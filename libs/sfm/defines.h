/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef SFM_DEFINES_HEADER
#define SFM_DEFINES_HEADER

#define SFM_NAMESPACE_BEGIN namespace sfm {
#define SFM_NAMESPACE_END }

#define SFM_BUNDLER_NAMESPACE_BEGIN namespace bundler {
#define SFM_BUNDLER_NAMESPACE_END }

#define SFM_PBA_NAMESPACE_BEGIN namespace pba {
#define SFM_PBA_NAMESPACE_END }

#define SFM_BA_NAMESPACE_BEGIN namespace ba {
#define SFM_BA_NAMESPACE_END }

#ifndef STD_NAMESPACE_BEGIN
#   define STD_NAMESPACE_BEGIN namespace std {
#   define STD_NAMESPACE_END }
#endif

/** Structure-from-Motion library. */
SFM_NAMESPACE_BEGIN
/** SfM bundler components. */
SFM_BUNDLER_NAMESPACE_BEGIN SFM_BUNDLER_NAMESPACE_END
/** Parallel Bundle Adjustment components. */
SFM_PBA_NAMESPACE_BEGIN SFM_PBA_NAMESPACE_END
SFM_NAMESPACE_END

#endif /* MVE_DEFINES_HEADER */
