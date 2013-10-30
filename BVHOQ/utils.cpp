#include "utils.h"
#include <cassert>
#include <fstream>

/// solve quadratic equation
bool	SolveQuadratic( const real& a, const real& b, const real&c, real& x1, real& x2 )
{
    real d = b*b - 4*a*c;
    if ( d < 0 )
        return false;
    else
    {
        real den = 1/(2*a);
        x1 = (-b - sqrt(d))*den;
        x2 = (-b + sqrt(d))*den;
        return true;
    }
}

matrix4x4 translation_matrix(vector3 const& v)
{
    return matrix4x4 (1, 0, 0, 0, 
        0, 1, 0, 0, 
        0, 0, 1, 0, 
        v.x(), v.y(), v.z(), 1);
}

matrix4x4 rotation_matrix_x(real const& ang)
{
    return matrix4x4(1, 0, 0, 0, 
        0, cos(ang), sin(ang), 0,
        0, -sin(ang), cos(ang), 0,
        0, 0, 0, 1);
}

matrix4x4 rotation_matrix_y(real const& ang)
{
    return matrix4x4(cos(ang), 0, -sin(ang), 0,
        0, 1, 0, 0,
        sin(ang), 0, cos(ang), 0,
        0, 0, 0, 1);

}

matrix4x4 rotation_matrix_z(real const& ang)
{
    return matrix4x4(cos(ang), sin(ang), 0, 0,
        -sin(ang), cos(ang), 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1);
}

matrix4x4 rotation_matrix(vector3 const& axis, real const& ang)
{
    // TODO: need to implement this
    assert(false);
    return matrix4x4();
}

matrix4x4 scale_matrix(vector3 const& v)
{
    return matrix4x4(v.x(), 0, 0, 0, 0, v.y(), 0, 0, 0, 0, v.z(), 0, 0, 0, 0, 1);
}

matrix4x4 perspective_proj_matrix_lh_dx(real const& l, real const& r, real const& b, real const& t, real const& n, real const& f)
{
    return matrix4x4(2*n/(r-l), 0, 0, 0, 
        0, 2*n/(t-b), 0, 0,
        -(r + l)/(r - l), -(t + b)/(t - b), f/(f - n), 1,
        0, 0, -f*n/(f - n), 0);  
}

matrix4x4 perspective_proj_matrix_lh_gl(real const& l, real const& r, real const& b, real const& t, real const& n, real const& f)
{
    return matrix4x4(2*n/(r-l), 0, 0, 0,
                     0, 2*n/(t-b), 0, 0,
                     -(r + l)/(r - l), -(t + b)/(t - b), (n + f)/(f - n), 1,
                     0, 0, -2*f*n/(f - n), 0);
}

matrix4x4 perspective_proj_fovy_matrix_lh_gl(real const& fovy, real const& aspect, real const& n, real const& f)
{
    real hH =  tan(fovy/2) * n;
    real hW  = hH * aspect;
    return perspective_proj_matrix_lh_gl( -hW, hW, -hH, hH, n, f);
}

matrix4x4 perspective_proj_matrix_rh_gl(real const& l, real const& r, real const& b, real const& t, real const& n, real const& f)
{
    return matrix4x4(2*n/(r-l), 0, 0, 0,
        0, 2*n/(t-b), 0, 0,
        (r + l)/(r - l), (t + b)/(t - b), -(f + n)/(f - n), -1,
        0, 0, -2*f*n/(f - n), 0);
}

matrix4x4 perspective_proj_fovy_matrix_lh_dx(real const& fovy, real const& aspect, real const& n, real const& f)
{
    real hH =  tan(fovy/2) * n;
    real hW  = hH * aspect;
    return perspective_proj_matrix_lh_dx( -hW, hW, -hH, hH, n, f);
}

matrix4x4 perspective_proj_fovy_matrix_rh_gl(real const& fovy, real const& aspect, real const& n, real const& f)
{
    real hH = tan(fovy) * n;
    real hW  = hH * aspect;
    return perspective_proj_matrix_rh_gl( -hW, hW, -hH, hH, n, f);
}

matrix4x4 lookat_matrix_lh_dx( vector3 const& pos, vector3 const& at, vector3 const& up)
{
    vector3 v = normalize(at - pos);
    vector3 r = cross(normalize(up), v);
    vector3 u = cross(v,r);
    vector3 ip = vector3(-dot(r,pos), -dot(u,pos), -dot(v,pos));
    
    return matrix4x4(r.x(), u.x(), v.x(), 0,
                     r.y(), u.y(), v.y(), 0,
                     r.z(), u.z(), v.z(), 0,
                     ip.x(), ip.y(), ip.z(), 1);
}

quat rotation_quat(vector3 const& axe, real const& angle )
{
    // create (sin(a/2)*axis, cos(a/2)) quaternion
    // which rotates the point a radians around "axis"
    quat res;
    vector4 u = vector4(axe.x(), axe.y(), axe.z(), 0); u.normalize();
    real sina2 = sin(angle/2);
    real cosa2 = cos(angle/2);

    res.qx() = sina2 * u.x();
    res.qy() = sina2 * u.y();
    res.qz() = sina2 * u.z();
    res.qw() = cosa2;

    return res;
}

vector3 rotate_vector( vector3 const& v, quat const& q )
{
    quat p = quat(v.x(), v.y(), v.z(), 0);
    quat tp = q * p * q.inverse();
    return vector3(tp.qx(), tp.qy(), tp.qz());
}

quat	rotate_quat( quat const& v, quat const& q )
{
    return q * v * q.inverse();
}

void load_file_contents(std::string const& name, std::vector<char>& contents, bool binary)
{
    std::ifstream in(name, std::ios::in | (binary?std::ios::binary : 0));

    if (in)
    {
        std::streamoff beg = in.tellg();

        in.seekg(0, std::ios::end);

        std::streamoff fileSize = in.tellg() - beg;

        in.seekg(0, std::ios::beg);

        contents.resize(static_cast<unsigned>(fileSize));

        in.read(&contents[0], fileSize);
    }
    else
    {
        throw std::runtime_error("Cannot read the contents of a file");
    }

}