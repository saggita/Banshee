/*
    Banshee and all code, documentation, and other materials contained
    therein are:

        Copyright 2013 Dmitry Kozlov
        All Rights Reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:
        * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
        * Neither the name of the software's owners nor the names of its
        contributors may be used to endorse or promote products derived from
        this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
    (This is the Modified BSD License)
*/

#ifndef QUATERNION_H
#define QUATERNION_H

#include <cmath>

class quaternion
{
public:
    quaternion (float xx = 0.f, float yy = 0.f, float zz = 0.f, float ww = 0.f) : x(xx), y(yy), z(zz), w(ww) {}

    //explicit			   quaternion( const vector<T,4>& v );
    /// create quaternion from a orthogonal(!) matrix
    /// make sure the matrix is ORTHOGONAL
    //explicit			   quaternion( const matrix<T,4,4>& m );

    /// convert quaternion to matrix
    //void				   to_matrix( matrix<T,4,4>& pM ) const;
    //matrix<T,4,4>	 to_matrix() const;

    quaternion      operator -() const { return quaternion(-x, -y, -z, -w); }
    quaternion&     operator +=( quaternion const& o ) { x+=o.x; y+=o.y; z+=o.z; w+=o.w; return *this; }
    quaternion&     operator -=( quaternion const& o ) { x-=o.x; y-=o.y; z-=o.z; w-=o.w; return *this; }
    quaternion&     operator *=( quaternion const& o ) { x*=o.x; y*=o.y; z*=o.z; w*=o.w; return *this; }
    quaternion&     operator *=( float a ) { x*=a; y*=a; z*=a; w*=a; return *this; }
    quaternion&     operator /=( float a ) { float inva = 1.f/a; x*=inva; y*=inva; z*=inva; w*=inva; return *this; }

    quaternion      conjugate() const { return quaternion(-x, -y, -z, w); }
    quaternion      inverse()   const; 

    float  sqnorm() const { return x * x + y * y + z * z + w * w; }
    float  norm()   const { return std::sqrt(sqnorm()); }

    float x, y, z, w;
};

inline quaternion      quaternion::inverse()   const
{
    if (sqnorm() == 0)
    {
        return quaternion();
    }
    else
    {
        quaternion q = conjugate();
        q /= sqnorm();
        return q;
    }
}

inline quaternion operator * (quaternion const& q1,  quaternion const& q2)
{
    quaternion res;
    res.x = q1.y*q2.z - q1.z*q2.y + q2.w*q1.x + q1.w*q2.x;
    res.y = q1.z*q2.x - q1.x*q2.z + q2.w*q1.y + q1.w*q2.y;
    res.z = q1.x*q2.y - q2.x*q1.y + q2.w*q1.z + q1.w*q2.z;
    res.w = q1.w*q2.w - q1.x*q2.x - q1.y*q2.y - q1.z*q2.z;
    return res;
}

inline quaternion operator / ( const quaternion& q,  float a )
{
    quaternion res = q;
    return res /= a;
}

inline quaternion operator + ( const quaternion& q1,  const quaternion& q2 )
{
    quaternion res = q1;
    return res += q2;
}

inline quaternion operator - ( const quaternion& q1,  const quaternion& q2 )
{
    quaternion res = q1;
    return res -= q2;
}

inline quaternion operator * ( const quaternion& q,  float a )
{
    quaternion res = q;
    return res *= a;
}

inline quaternion operator * ( float a, quaternion const& q )
{
    quaternion res = q;
    return res *= a;
}

inline quaternion normalize( quaternion const& q )
{
    float norm = q.norm();
    return q / norm;
}

#endif // QUATERNION_H