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

#include "sfm/defines.h"
#include "util/aligned_allocator.h"
#include "sfm/pba_config.h"
#include "sfm/pba_types.h"

//BYTE-ALIGNMENT for data allocation (16 required for SSE, 32 required for AVX)
//PREVIOUS version uses only SSE. The new version will include AVX.
//SO the alignment is increased from 16 to 32
#define VECTOR_ALIGNMENT 32
#define FLOAT_ALIGN      8
#define VECTOR_ALIGNMENT_MASK (VECTOR_ALIGNMENT - 1)
#define ALIGN_PTR(p)	  (( ((size_t) p) + VECTOR_ALIGNMENT_MASK)  & (~VECTOR_ALIGNMENT_MASK))

SFM_NAMESPACE_BEGIN
SFM_PBA_NAMESPACE_BEGIN

class avec
{
    typedef util::AlignedAllocator<double, VECTOR_ALIGNMENT> Allocator;
    Allocator allocator;

    bool _owner;
    double* _data;
    double* _last;
    size_t _size;
    size_t _capacity;

public:
    avec()
    {
        _owner = true;
        _last = _data = nullptr;
        _size = _capacity = 0;
    }
    avec(size_t count)
    {
        _data = allocator.allocate(count);
        _size = _capacity = count;
        _last = _data + count;
        _owner = true;
    }
    ~avec()
    {
        if(_data && _owner) allocator.deallocate(_data, _capacity);
    }

    inline void resize(size_t newcount)
    {
        if(!_owner)
        {
            _data = _last = nullptr;
            _capacity = _size = 0;
            _owner = true;
        }
        if(newcount <= _capacity)
        {
            _size = newcount;
            _last = _data + newcount;
        }else
        {
            if(_data && _owner) allocator.deallocate(_data, _capacity);
            _data = allocator.allocate(newcount);
            _size = _capacity = newcount;
            _last = _data + newcount;
        }
    }

    inline void set(double* data, size_t size)
    {
        if(_data && _owner) allocator.deallocate(_data, _capacity);
        _data = data;
        _owner = false;
        _size = size;
        _last = _data + _size;
        _capacity = size;
    }
    inline void swap(avec& next)
    {
        bool _owner_bak = _owner;
        double* _data_bak = _data;
        double* _last_bak = _last;
        size_t _size_bak = _size;
        size_t _capa_bak = _capacity;

        _owner = next._owner;
        _data = next._data;
        _last = next._last;
        _size = next._size;
        _capacity = next._capacity;

        next._owner = _owner_bak;
        next._data = _data_bak;
        next._last = _last_bak;
        next._size = _size_bak;
        next._capacity = _capa_bak;

    }

    inline operator double* () {return _size? _data : nullptr;}
    inline operator const double* () const {return _data; }
    inline double* begin() {return _size? _data : nullptr;}
    inline double* data() {return _size? _data : nullptr;}
    inline double* end() {return _last;}
    inline const double* begin() const {return _size? _data : nullptr;}
    inline const double* end() const {return _last;}
    inline size_t size() const {return _size;}
    inline size_t IsValid() const {return _size;}
};

class SparseBundleCPU : public ConfigBA
{
public:
    typedef avec   VectorF;
    typedef std::vector<int>   VectorI;

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
    void		ProcessWeightCameraQ(std::vector<int>&cpnum, std::vector<int>&qmap, double* qmapw, double* qlistw);

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

SFM_PBA_NAMESPACE_END
SFM_NAMESPACE_END

#endif // SFM_PBA_CPU_HEADER
