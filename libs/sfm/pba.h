/*
 * Copyright (c) 2011  Changchang Wu (ccwu@cs.washington.edu)
 *    and the University of Washington at Seattle
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  Version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 */

// TODO (Simon Fuhrmann)
// - Reorganize classes (public, protected, private)
// - Remove specialization for float, always use double?

#ifndef SFM_PBA_HEADER
#define SFM_PBA_HEADER

//filetype definitions for points and camera
#include "pba_types.h"
#include "pba_config.h"

SFM_NAMESPACE_BEGIN

class ParallelBA
{
public:
    enum StatusT
    {
        STATUS_SUCCESS = 0,
        STATUS_CAMERA_MISSING = 1,
        STATUS_POINT_MISSING,
        STATUS_PROJECTION_MISSING,
        STATUS_MEASURMENT_MISSING,
        STATUS_ALLOCATION_FAIL
    };

    enum DeviceT
    {
        PBA_INVALID_DEVICE = -4,
        PBA_CPU_DOUBLE = -3,
        PBA_CPU_FLOAT = -2,
        PBA_CUDA_DEVICE_DEFAULT = -1,
        PBA_CUDA_DEVICE0 = 0
    };

    enum DistortionT
    {
        PBA_MEASUREMENT_DISTORTION = -1, //single parameter, apply to measurements
        PBA_NO_DISTORTION = 0,           //no radial distortion
        PBA_PROJECTION_DISTORTION = 1    //single parameter, apply to projectino
    };

    enum BundleModeT
    {
        BUNDLE_FULL = 0,
        BUNDLE_ONLY_MOTION = 1,
        BUNDLE_ONLY_STRUCTURE = 2
    };

public:
    ////////////////////////////////////////////////////
    //methods for changing bundle adjustment settings
    virtual void ParseParam(int narg, char** argv);           //indirect method
    virtual ConfigBA* GetInternalConfig();                    //direct method
    virtual void SetFixedIntrinsics(bool fixed);              //call this for calibrated system
    virtual void EnableRadialDistortion(DistortionT type);    //call this to enable radial distortion
    virtual void SetNextTimeBudget(int seconds);              //# of seconds for next run (0 = no limit)
    virtual void ReserveStorage(size_t ncam, size_t npt, size_t nproj);

public:
    //function name change; the old one is mapped as inline function
    inline void SetFocalLengthFixed(bool fixed) {SetFixedIntrinsics(fixed); }
    inline void ResetBundleStorage() {ReserveStorage(0, 0, 0); /*Reset devide for CUDA*/ }

public:
    /////////////////////////////////////////////////////
    //optimizer interface, input and run
    virtual void SetCameraData(size_t ncam,  CameraT* cams);			//set camera data
    virtual void SetPointData(size_t npoint, Point3D* pts);			//set 3D point data
    virtual void SetProjection(size_t nproj,
        const Point2D* imgpts,  const int* point_idx, const int* cam_idx);		//set projections
    virtual void SetNextBundleMode(BundleModeT mode = BUNDLE_FULL);	//mode of the next bundle adjustment call
    virtual int  RunBundleAdjustment();								//start bundle adjustment, return number of successful LM iterations

public:
    //////////////////////////////////////////////////
    //Query optimzer runing status for Multi-threading
    //Three functions below can be called from a differnt thread while bundle is running
    virtual float GetMeanSquaredError();        //read back results during/after BA
    virtual void  AbortBundleAdjustment();      //tell bundle adjustment to abort ASAP
    virtual int   GetCurrentIteration();        //which iteration is it working on?

public:
    ParallelBA(DeviceT device = PBA_CUDA_DEVICE_DEFAULT);
    void* operator new (size_t size);
    virtual ~ParallelBA();

public:
    //////////////////////////////////////////////
    //Future functions will be added to the end for compatiability with old version.
    virtual void SetFocalMask(const int * fmask, float weight = 1.0f);

private:
    ParallelBA *   _optimizer;
};

//function for dynamic loading of library
ParallelBA * NewParallelBA(ParallelBA::DeviceT device = ParallelBA::PBA_CUDA_DEVICE_DEFAULT);
typedef ParallelBA * (*NEWPARALLELBAPROC)(ParallelBA::DeviceT);

///////////////////////////////////////////////
//older versions do not have this function.
int  ParallelBA_GetVersion();

SFM_NAMESPACE_END

#endif
