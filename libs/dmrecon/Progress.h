#ifndef MVS_PROGRESS_H
#define MVS_PROGRESS_H

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
    size_t filled; ///< amount of pixels with reconstructed depth value
    unsigned int queueSize; ///< current size of MVS pixel queue
    ReconStatus status; ///< current status of MVS algorithm
    bool cancelled; ///< set from extern to true to cancel reconstruction
    size_t start_time; ///< start time of MVS reconstruction, or 0

    Progress()
        :
        filled(0),
        queueSize(0),
        status(RECON_IDLE),
        cancelled(false),
        start_time(0)
    {
    }
};

MVS_NAMESPACE_END

#endif
