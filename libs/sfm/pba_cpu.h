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

#ifndef SFM_PBA_CPU_HEADER
#define SFM_PBA_CPU_HEADER

#include <malloc.h>
#include "sfm/defines.h"
#include "util/aligned_allocator.h"

//BYTE-ALIGNMENT for data allocation (16 required for SSE, 32 required for AVX)
//PREVIOUS version uses only SSE. The new version will include AVX.
//SO the alignment is increased from 16 to 32
#define VECTOR_ALIGNMENT 32
#define FLOAT_ALIGN      8
#define VECTOR_ALIGNMENT_MASK (VECTOR_ALIGNMENT - 1)
#define ALIGN_PTR(p)	  (( ((size_t) p) + VECTOR_ALIGNMENT_MASK)  & (~VECTOR_ALIGNMENT_MASK))

SFM_NAMESPACE_BEGIN

template <class Float>
class avec : public std::vector<Float, util::AlignedAllocator<Float, VECTOR_ALIGNMENT> >
{
public:
    avec() {}
    avec(size_t sz) : std::vector<Float, util::AlignedAllocator<Float, VECTOR_ALIGNMENT> >(sz) {}

    inline void set(Float* data, size_t size)
    {
        this->assign(data, data+size);
    }

    inline operator Float* ()               {return this->empty() ? NULL : &this->front();}
    inline operator const Float* () const   {return this->empty() ? NULL : &this->front();}
    inline Float* begin()                   {return this->empty() ? NULL : &this->front();}
    inline Float* end()                     {return this->empty() ? NULL : &this->back()+1;}
    inline const Float* begin() const       {return this->empty() ? NULL : &this->front();}
    inline const Float* end()   const       {return this->empty() ? NULL : &this->back()+1;}
    void   SaveToFile(const char* name);
};

template<class Float>
class SparseBundleCPU : public ParallelBA, public ConfigBA
{
public:
    typedef avec<Float>   VectorF;
    typedef std::vector<int>   VectorI;
    typedef float         float_t;

protected:      //cpu data
    int             _num_camera;
    int             _num_point;
    int             _num_imgpt;
    CameraT*        _camera_data;
    float*          _point_data;

    ////////////////////////////////
    const float*    _imgpt_data;
    const int*      _camera_idx;
    const int*      _point_idx;
    const int*		_focal_mask;

    ///////////sumed square error
    float         _projection_sse;

protected:      //cuda data
    VectorF      _cuCameraData;
    VectorF      _cuCameraDataEX;
    VectorF      _cuPointData;
    VectorF      _cuPointDataEX;
    VectorF      _cuMeasurements;
    VectorF      _cuImageProj;
    VectorF      _cuJacobianCamera;
    VectorF      _cuJacobianPoint;
    VectorF      _cuJacobianCameraT;
    VectorI      _cuProjectionMap;
    VectorI      _cuPointMeasurementMap;
    VectorI      _cuCameraMeasurementMap;
    VectorI      _cuCameraMeasurementList;
    VectorI      _cuCameraMeasurementListT;

    //////////////////////////
    VectorF      _cuBlockPC;
    VectorF      _cuVectorSJ;

    ///LM normal    equation
    VectorF      _cuVectorJtE;
    VectorF      _cuVectorJJ;
    VectorF      _cuVectorJX;
    VectorF      _cuVectorXK;
    VectorF      _cuVectorPK;
    VectorF      _cuVectorZK;
    VectorF      _cuVectorRK;

    //////////////////////////////////
protected:
    int          _num_imgpt_q;
    float		 _weight_q;
    VectorI		 _cuCameraQList;
    VectorI		 _cuCameraQMap;
    VectorF		 _cuCameraQMapW;
    VectorF		 _cuCameraQListW;
protected:
    bool		ProcessIndexCameraQ(std::vector<int>&qmap, std::vector<int>& qlist);
    void		ProcessWeightCameraQ(std::vector<int>&cpnum, std::vector<int>&qmap, Float* qmapw, Float* qlistw);

protected:      //internal functions
    int         ValidateInputData();
    int         InitializeBundle();
    int         GetParameterLength();
    void        BundleAdjustment();
    void        NormalizeData();
    void        TransferDataToHost();
    void        DenormalizeData();
    void        NormalizeDataF();
    void        NormalizeDataD();
    bool        InitializeStorageForSFM();
    bool        InitializeStorageForCG();

    void        SaveBundleRecord(int iter, float res, float damping, float& g_norm, float& g_inf);
protected:
    void        PrepareJacobianNormalization();
    void        EvaluateJacobians();
    void        ComputeJtE(VectorF& E, VectorF& JtE, int mode = 0);
    void        ComputeJX(VectorF& X, VectorF& JX, int mode = 0);
    void        ComputeDiagonal(VectorF& JJI);
    void        ComputeBlockPC(float lambda, bool dampd);
    void        ApplyBlockPC(VectorF& v, VectorF& pv, int mode = 0);
    float       UpdateCameraPoint(VectorF& dx, VectorF& cuImageTempProj);
    float       EvaluateProjection(VectorF& cam, VectorF&point, VectorF& proj);
    float       EvaluateProjectionX(VectorF& cam, VectorF&point, VectorF& proj);
    float		SaveUpdatedSystem(float residual_reduction, float dx_sqnorm, float damping);
    float		EvaluateDeltaNorm();
    int         SolveNormalEquationPCGB(float lambda);
    int         SolveNormalEquationPCGX(float lambda);
    int			SolveNormalEquation(float lambda);
    void        NonlinearOptimizeLM();
    void		AdjustBundleAdjsutmentMode();
    void        RunProfileSteps();
    void        RunTestIterationLM(bool reduced);
    void        DumpCooJacobian();
private:
    static int	FindProcessorCoreNum();
public:

    virtual void AbortBundleAdjustment()                    {__abort_flag = true;}
    virtual int  GetCurrentIteration()                      {return __current_iteration; }
    virtual void SetNextTimeBudget(int seconds)             {__bundle_time_budget = seconds;}
    virtual void SetNextBundleMode(BundleModeT mode)		{__bundle_mode_next = mode;}
    virtual void SetFixedIntrinsics(bool fixed)            {__fixed_intrinsics = fixed; }
    virtual void EnableRadialDistortion(DistortionT type)   {__use_radial_distortion = type; }
    virtual void ParseParam(int narg, char** argv)          {ConfigBA::ParseParam(narg, argv); }
    virtual ConfigBA* GetInternalConfig()                   {return this; }
public:
    SparseBundleCPU();
    virtual void SetCameraData(size_t ncam,  CameraT* cams);
    virtual void SetPointData(size_t npoint, Point3D* pts);
    virtual void SetProjection(size_t nproj, const Point2D* imgpts, const int* point_idx, const int* cam_idx);
    virtual void SetFocalMask(const int* fmask, float weight);
    virtual float GetMeanSquaredError();
    virtual int RunBundleAdjustment();
};

ParallelBA* NewSparseBundleCPU(bool dp);

SFM_NAMESPACE_END

#endif // SFM_PBA_CPU_HEADER
