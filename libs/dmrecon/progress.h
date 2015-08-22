/*
 * Copyright (C) 2015, Ronny Klowsky, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef DMRECON_PROGRESS_H
#define DMRECON_PROGRESS_H

#include "dmrecon/defines.h"

MVS_NAMESPACE_BEGIN

enum ReconStatus
{
    RECON_IDLE,
    RECON_GLOBALVS,
    RECON_FEATURES,
    RECON_QUEUE,
    RECON_SAVING,
    RECON_CANCELLED
};

struct Progress
{
    ReconStatus status; ///< current status of MVS algorithm
    std::size_t filled; ///< amount of pixels with reconstructed depth value
    std::size_t queueSize; ///< current size of MVS pixel queue
    std::size_t start_time; ///< start time of MVS reconstruction, or 0
    bool cancelled; ///< set from extern to true to cancel reconstruction

    Progress()
        : status(RECON_IDLE)
        , filled(0)
        , queueSize(0)
        , start_time(0)
        , cancelled(false)
    {
    }
};

MVS_NAMESPACE_END

#endif
