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