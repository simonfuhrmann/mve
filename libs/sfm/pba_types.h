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

#ifndef SFM_PBA_TYPES_HEADER
#define SFM_PBA_TYPES_HEADER

#include <math.h>
#include "sfm/defines.h"

#define PBA_PI 3.14159265358979323846

// ----------------------------WARNING------------------------------
// -----------------------------------------------------------------
// ROTATION CONVERSION:
// The internal rotation representation is 3x3 float matrix. Reading
// back the rotations as quaternion or Rodrigues's representation will
// cause inaccuracy, IF you have wrongly reconstructed cameras with
// a very very large focal length (typically also very far away).
// In this case, any small change in the rotation matrix, will cause
// a large reprojection error.
//
// ---------------------------------------------------------------------
// RADIAL distortion is NOT enabled by default, use parameter "-md", -pd"
// or set ConfigBA::__use_radial_distortion to 1 or -1 to enable it.
// ---------------------------------------------------------------------------

SFM_NAMESPACE_BEGIN
SFM_PBA_NAMESPACE_BEGIN

struct CameraT
{
    //////////////////////////////////////////////////////
    float		f;					// single focal length, K = [f, 0, 0; 0 f 0; 0 0 1]
    float		t[3];				// T in  P = K[R T], T = - RC
    float		m[3][3];			// R in  P = K[R T].
    float		radial;				// WARNING: BE careful with the radial distortion model.
    int			distortion_type;
    float		constant_camera;

    //////////////////////////////////////////////////////////
    CameraT(){    radial = 0;  distortion_type = 0;  constant_camera = 0;  }

    //////////////////////////////////////////////
    void SetCameraT(const CameraT& cam)
    {
        f = (float) cam.f;
        t[0] = (float)cam.t[0];    t[1] = (float)cam.t[1];    t[2] = (float)cam.t[2];
        for(int i = 0; i < 3; ++i) for(int j = 0; j < 3; ++j) m[i][j] = (float)cam.m[i][j];
        radial = (float) cam.radial;
        distortion_type = cam.distortion_type;
        constant_camera = cam.constant_camera;
    }

    //////////////////////////////////////////
    void SetConstantCamera()	{constant_camera = 1.0f;}
    void SetVariableCamera()    {constant_camera = 0.0f;}
    void SetFixedIntrinsic()	{constant_camera = 2.0f;}
    //void SetFixedExtrinsic()	{constant_camera = 3.0f;}

    //////////////////////////////////////
    void SetFocalLength(float F){        f = (float) F;    }
    float GetFocalLength()	const{return f;}

    void SetMeasumentDistortion(float r)    {radial = (float) r; distortion_type = -1;}
    float GetMeasurementDistortion() const {return distortion_type == -1 ? radial : 0; }

    //normalize radial distortion that applies to angle will be (radial * f * f);
    void SetNormalizedMeasurementDistortion(float r)  {SetMeasumentDistortion(r / (f * f)); }
    float GetNormalizedMeasurementDistortion()  const{return GetMeasurementDistortion() * (f * f); }

    //use projection distortion
    void SetProjectionDistortion(float r)   {radial = float(r);  distortion_type = 1; }
    void SetProjectionDistortion(const float* r)   {SetProjectionDistortion(r[0]); }
    float GetProjectionDistortion()  {return distortion_type == 1 ? radial : 0; }

    void SetRodriguesRotation(const float r[3])
    {
        double a = sqrt(r[0]*r[0]+r[1]*r[1]+r[2]*r[2]);
        double ct = a==0.0?0.5:(1.0-cos(a))/a/a;
        double st = a==0.0?1:sin(a)/a;
        m[0][0]=float(1.0 - (r[1]*r[1] + r[2]*r[2])*ct);
        m[0][1]=float(r[0]*r[1]*ct - r[2]*st);
        m[0][2]=float(r[2]*r[0]*ct + r[1]*st);
        m[1][0]=float(r[0]*r[1]*ct + r[2]*st);
        m[1][1]=float(1.0 - (r[2]*r[2] + r[0]*r[0])*ct);
        m[1][2]=float(r[1]*r[2]*ct - r[0]*st);
        m[2][0]=float(r[2]*r[0]*ct - r[1]*st);
        m[2][1]=float(r[1]*r[2]*ct + r[0]*st);
        m[2][2]=float(1.0 - (r[0]*r[0] + r[1]*r[1])*ct );
    }
    void GetRodriguesRotation(float r[3]) const
    {
        double a = (m[0][0]+m[1][1]+m[2][2]-1.0)/2.0;
        const double epsilon = 0.01;
        if( fabs(m[0][1] - m[1][0]) < epsilon &&
            fabs(m[1][2] - m[2][1]) < epsilon &&
            fabs(m[0][2] - m[2][0]) < epsilon )
        {
            if( fabs(m[0][1] + m[1][0]) < 0.1 &&
                fabs(m[1][2] + m[2][1]) < 0.1 &&
                fabs(m[0][2] + m[2][0]) < 0.1 && a > 0.9)
            {
                r[0]    =    0;
                r[1]    =    0;
                r[2]    =    0;
            }
            else
            {
                const float ha = float(sqrt(0.5) * PBA_PI);
                double xx = (m[0][0]+1.0)/2.0;
                double yy = (m[1][1]+1.0)/2.0;
                double zz = (m[2][2]+1.0)/2.0;
                double xy = (m[0][1]+m[1][0])/4.0;
                double xz = (m[0][2]+m[2][0])/4.0;
                double yz = (m[1][2]+m[2][1])/4.0;

                if ((xx > yy) && (xx > zz))
                {
                    if (xx< epsilon)
                    {
                        r[0] = 0;    r[1] = r[2] = ha;
                    } else
                    {
                        double t = sqrt(xx) ;
                        r[0] = float(t * PBA_PI);
                        r[1] = float(xy/t * PBA_PI);
                        r[2] = float(xz/t * PBA_PI);
                    }
                } else if (yy > zz)
                {
                    if (yy< epsilon)
                    {
                        r[0] = r[2]  = ha; r[1] = 0;
                    } else
                    {
                        double t = sqrt(yy);
                        r[0] = float(xy/t* PBA_PI);
                        r[1] = float( t * PBA_PI);
                        r[2] = float(yz/t* PBA_PI);
                    }
                } else
                {
                    if (zz< epsilon)
                    {
                        r[0] = r[1] = ha; r[2] = 0;
                    } else
                    {
                        double t  = sqrt(zz);
                        r[0]  = float(xz/ t* PBA_PI);
                        r[1]  = float(yz/ t* PBA_PI);
                        r[2]  = float( t * PBA_PI);
                    }
                }
            }
        }
        else
        {
            a = acos(a);
            double b = 0.5*a/sin(a);
            r[0]    =    float(b*(m[2][1]-m[1][2]));
            r[1]    =    float(b*(m[0][2]-m[2][0]));
            r[2]    =    float(b*(m[1][0]-m[0][1]));
        }
    }
    ////////////////////////
    void SetQuaternionRotation(const float q[4])
    {
        double qq = sqrt(q[0]*q[0]+q[1]*q[1]+q[2]*q[2]+q[3]*q[3]);
        double qw, qx, qy, qz;
        if(qq>0)
        {
            qw=q[0]/qq;
            qx=q[1]/qq;
            qy=q[2]/qq;
            qz=q[3]/qq;
        }else
        {
            qw = 1;
            qx = qy = qz = 0;
        }
        m[0][0]=float(qw*qw + qx*qx- qz*qz- qy*qy );
        m[0][1]=float(2*qx*qy -2*qz*qw );
        m[0][2]=float(2*qy*qw + 2*qz*qx);
        m[1][0]=float(2*qx*qy+ 2*qw*qz);
        m[1][1]=float(qy*qy+ qw*qw - qz*qz- qx*qx);
        m[1][2]=float(2*qz*qy- 2*qx*qw);
        m[2][0]=float(2*qx*qz- 2*qy*qw);
        m[2][1]=float(2*qy*qz + 2*qw*qx );
        m[2][2]=float(qz*qz+ qw*qw- qy*qy- qx*qx);
    }
    void GetQuaternionRotation(float q[4]) const
    {
        q[0]= 1 + m[0][0] + m[1][1] + m[2][2];
        if(q[0]>0.000000001)
        {
            q[0] = sqrt(q[0])/2.0;
            q[1]= (m[2][1] - m[1][2])/( 4.0 *q[0]);
            q[2]= (m[0][2] - m[2][0])/( 4.0 *q[0]);
            q[3]= (m[1][0] - m[0][1])/( 4.0 *q[0]);
        }else
        {
            double s;
            if ( m[0][0] > m[1][1] && m[0][0] > m[2][2] )
            {
              s = 2.0 * sqrt( 1.0 + m[0][0] - m[1][1] - m[2][2]);
              q[1] = 0.25 * s;
              q[2] = (m[0][1] + m[1][0] ) / s;
              q[3] = (m[0][2] + m[2][0] ) / s;
              q[0] = (m[1][2] - m[2][1] ) / s;
            } else if (m[1][1] > m[2][2])
            {
              s = 2.0 * sqrt( 1.0 + m[1][1] - m[0][0] - m[2][2]);
              q[1] = (m[0][1] + m[1][0] ) / s;
              q[2] = 0.25 * s;
              q[3] = (m[1][2] + m[2][1] ) / s;
              q[0] = (m[0][2] - m[2][0] ) / s;
            } else
            {
              s = 2.0 * sqrt( 1.0 + m[2][2] - m[0][0] - m[1][1] );
              q[1] = (m[0][2] + m[2][0] ) / s;
              q[2] = (m[1][2] + m[2][1] ) / s;
              q[3] = 0.25f * s;
              q[0] = (m[0][1] - m[1][0] ) / s;
            }
        }
    }
    ////////////////////////////////////////////////
    void SetMatrixRotation(const float * r)
    {
        for(int i = 0; i < 9; ++i) m[0][i] = float(r[i]);
    }
    void GetMatrixRotation(float * r) const
    {
        for(int i = 0; i < 9; ++i) r[i] = float(m[0][i]);
    }
    float GetRotationMatrixDeterminant()const
    {
         return m[0][0]*m[1][1]*m[2][2] +
                m[0][1]*m[1][2]*m[2][0] +
                m[0][2]*m[1][0]*m[2][1] -
                m[0][2]*m[1][1]*m[2][0] -
                m[0][1]*m[1][0]*m[2][2] -
                m[0][0]*m[1][2]*m[2][1];
    }
    ///////////////////////////////////////
    void SetTranslation(const float T[3])
    {
        t[0] = (float)T[0];
        t[1] = (float)T[1];
        t[2] = (float)T[2];
    }
    void GetTranslation(float T[3])  const
    {
        T[0] = (float)t[0];
        T[1] = (float)t[1];
        T[2] = (float)t[2];
    }
    /////////////////////////////////////////////
    void SetCameraCenterAfterRotation(const float c[3])
    {
        //t = - R * C
        for(int j = 0; j < 3; ++j) t[j] = -float(m[j][0] * c[0] + m[j][1] * c[1] + m[j][2] * c[2]);
    }
    void GetCameraCenter(float c[3])
    {
        //C = - R' * t
        for(int j = 0; j < 3; ++j) c[j] = -float(m[0] [j]* t[0] + m[1][j] * t[1] + m[2][j] * t[2]);
    }
    ////////////////////////////////////////////
    void SetInvertedRT(const float e[3], const float T[3])
    {
        SetRodriguesRotation(e);
        for(int i = 3; i < 9; ++i) m[0][i] = - m[0][i];
        SetTranslation(T); t[1] = - t[1]; t[2] = -t[2];
    }

    void GetInvertedRT (float e[3], float T[3]) const
    {
        CameraT ci;    ci.SetMatrixRotation(m[0]);
        for(int i = 3; i < 9; ++i) ci.m[0][i] = - ci.m[0][i];
        //for(int i = 1; i < 3; ++i) for(int j = 0; j < 3; ++j) ci.m[i][j] = - ci.m[i][j];
        ci.GetRodriguesRotation(e);
        GetTranslation(T);    T[1] = - T[1]; T[2] = -T[2];
    }
    void SetInvertedR9T(const float e[9], const float T[3])
    {
        //for(int i = 0; i < 9; ++i) m[0][i] = (i < 3 ? e[i] : - e[i]);
        //SetTranslation(T); t[1] = - t[1]; t[2] = -t[2];
        m[0][0] = e[0];        m[0][1] = e[1];      m[0][2] = e[2];
        m[1][0] = -e[3];       m[1][1] = -e[4];     m[1][2] = -e[5];
        m[2][0] = -e[6];       m[2][1] = -e[7];     m[2][2] = -e[8];
        t[0] = T[0];           t[1] = -T[1];        t[2] = -T[2];
    }
    void GetInvertedR9T(float e[9], float T[3]) const
    {
        e[0] = m[0][0];        e[1] = m[0][1];      e[2] = m[0][2];
        e[3] = - m[1][0];      e[4] = -m[1][1];     e[5] = -m[1][2];
        e[6] = -m[2][0];       e[7] = -m[2][1];     e[8] = -m[2][2] ;
        T[0] = t[0];           T[1] = -t[1];		T[2] = -t[2];
    }
};

struct Point3D
{
    float xyz[3];    //3D point location
    float reserved;    //alignment
};

struct Point2D
{
    float x, y;
};

SFM_PBA_NAMESPACE_END
SFM_NAMESPACE_END

#endif // SFM_PBA_TYPES_HEADER
