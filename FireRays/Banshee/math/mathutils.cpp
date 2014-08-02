#include "mathutils.h"

#include <cassert>
#include <fstream>

bool	solve_quadratic( float a, float b, float c, float& x1, float& x2 )
{
    float d = b*b - 4*a*c;
    if ( d < 0 )
        return false;
    else
    {
        float den = 1/(2*a);
        x1 = (-b - std::sqrt(d))*den;
        x2 = (-b + std::sqrt(d))*den;
        return true;
    }
}

matrix translation(float3 const& v)
{
    return matrix (1, 0, 0, v.x, 
                   0, 1, 0, v.y, 
                   0, 0, 1, v.z, 
                   0, 0, 0, 1);
}

matrix rotation_x(float ang)
{
    return matrix(1, 0, 0, 0, 
                  0, std::cos(ang), -std::sin(ang), 0,
                  0, std::sin(ang), std::cos(ang), 0,
                  0, 0, 0, 1);
}

matrix rotation_y(float ang)
{
    return matrix(std::cos(ang), 0, std::sin(ang), 0,
                  0, 1, 0, 0,
                  -std::sin(ang), 0, std::cos(ang), 0,
                  0, 0, 0, 1);
}

matrix rotation_z(float ang)
{
    return matrix(std::cos(ang), -std::sin(ang), 0, 0,
                 std::sin(ang), std::cos(ang), 0, 0,
                 0, 0, 1, 0,
                 0, 0, 0, 1);
}

matrix rotation(float3 const& axis, float ang)
{
    assert(false);
    return matrix();
}

matrix scale(float3 const& v)
{
    return matrix(v.x, 0, 0, 0, 0, v.y, 0, 0, 0, 0, v.z, 0, 0, 0, 0, 1.f);
}

matrix perspective_proj_lh_dx(float l, float r, float b, float t, float n, float f)
{
    return matrix(2*n/(r-l), 0, 0, 0, 
                  0, 2*n/(t-b), 0, -(r + l)/(r - l),
                  0, -(t + b)/(t - b), f/(f - n), -f*n/(f - n),
                  0, 0, 1, 0);  
}

matrix perspective_proj_lh_gl(float l, float r, float b, float t, float n, float f)
{
    return matrix(2*n/(r-l), 0, 0, 0,
                  0, 2*n/(t-b), 0, -(r + l)/(r - l),
                  0, -(t + b)/(t - b), -2*f*n/(f - n), 1,
                  0, 0, (n + f)/(f - n), 0);
}

matrix perspective_proj_fovy_lh_gl(float fovy, float aspect, float n, float f)
{
    float hH =  tan(fovy/2) * n;
    float hW  = hH * aspect;
    return perspective_proj_lh_gl( -hW, hW, -hH, hH, n, f);
}

matrix perspective_proj_rh_gl(float l, float r, float b, float t, float n, float f)
{
    return matrix(2*n/(r-l), 0, 0, 0,
        0, 2*n/(t-b), 0, (r + l)/(r - l),
        0, (t + b)/(t - b), -(f + n)/(f - n), -2*f*n/(f - n),
        0, 0, -1, 0);
}

matrix perspective_proj_fovy_lh_dx(float fovy, float aspect, float n, float f)
{
    float hH =  tan(fovy/2) * n;
    float hW  = hH * aspect;
    return perspective_proj_lh_dx( -hW, hW, -hH, hH, n, f);
}

matrix perspective_proj_fovy_rh_gl(float fovy, float aspect, float n, float f)
{
    float hH = tan(fovy) * n;
    float hW  = hH * aspect;
    return perspective_proj_rh_gl( -hW, hW, -hH, hH, n, f);
}

matrix lookat_lh_dx( float3 const& pos, float3 const& at, float3 const& up)
{
    float3 v = normalize(at - pos);
    float3 r = cross(normalize(up), v);
    float3 u = cross(v,r);
    float3 ip = float3(-dot(r,pos), -dot(u,pos), -dot(v,pos));

    return matrix(r.x, r.y, r.z, ip.x,
        u.x, u.y, u.z, ip.y,
        v.x, v.y, v.z, ip.z,
        0, 0, 0, 1);
}

quaternion rotation_quaternion(float3 const& axe, float angle)
{
    // create (sin(a/2)*axis, cos(a/2)) quaternion
    // which rotates the point a radians around "axis"
    quaternion res;
    float3 u = axe; u.normalize();
    float sina2 = std::sin(angle/2);
    float cosa2 = std::cos(angle/2);

    res.x = sina2 * u.x;
    res.y = sina2 * u.y;
    res.z = sina2 * u.z;
    res.w = cosa2;

    return res;
}

float3 rotate_vector(float3 const& v, quaternion const& q)
{
    quaternion p = quaternion(v.x, v.y, v.z, 0);
    quaternion tp = q * p * q.inverse();
    return float3(tp.x, tp.y, tp.z);
}

quaternion	rotate_quaternion( quaternion const& v, quaternion const& q )
{
    return q * v * q.inverse();
}