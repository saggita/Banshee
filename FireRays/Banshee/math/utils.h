/////////////////////////////////////////////////////
/// @file utils.h
///	 Various utility functions
///
/// @author Dmitry Kolzov
///
#ifndef utils_H
#define utils_H

#include "common_types.h"
#include <time.h>
#include <stdlib.h>
#include <string>
#include <vector>

#define OFFSETOF(struc,member) (&(((struc*)0)->member))


/// Convert 3d vector to homogenous point to apply trasnform to
inline vector4 vector_to_point( const vector3& v ) { return vector4(v.x(), v.y(), v.z(), real(1)); }

/// Convert 3d vector to homogenous vector to apply transform to
/// Note that normal transforms should be declared explicitly 
inline vector4 vector_to_vector( const vector3& v ) { return vector4(v.x(), v.y(), v.z(), real(0)); }

/// Initialize RNG
inline void	rand_init() {srand((unsigned)time(0));}

/// Generate random float value within [0,1] range
inline real	rand_float() { return (real)rand()/RAND_MAX; }

/// Genarate random uint value
inline uint	rand_uint() { return (uint)rand(); }

/// Convert cartesian coordinates to spherical
inline void	cartesian_to_spherical ( const vector3& cart, real& r, real& phi, real& theta )
{
	real temp = atan2(cart.x(), cart.z());
	r = sqrt(cart.x()*cart.x() + cart.y()*cart.y() + cart.z()*cart.z());
	phi = (real)((temp >= 0)?temp:(temp + 2*M_PI));
	theta = acos(cart.y()/r);
}

/// Convert cartesian coordinates to spherical
inline void	cartesian_to_spherical ( const vector3& cart, vector3& sph ) 
{
	cartesian_to_spherical(cart, sph.x(), sph.y(), sph.z());
}

/// Clamp float value to [a, b) range
inline float clamp(float x, float a, float b)

{
    return x < a ? a : (x > b ? b : x);
}

/// Clamp each component of the vector to [a, b) range
inline vector3 clamp(vector3 const& v, vector3 const& v1, vector3 const& v2)
{
    vector3 res;
    res.x() = clamp(v.x(), v1.x(), v2.x());
    res.y() = clamp(v.y(), v1.y(), v2.y());
    res.z() = clamp(v.z(), v1.z(), v2.z());
    return res;
}

/// Convert spherical coordinates to cartesian 
inline void	spherical_to_cartesian ( const real& r, const real& phi, const real& theta, vector3& cart )
{
	cart.y() = r * cos(theta);
	cart.z() = r * sin(theta) * cos(phi);
	cart.x() = r * sin(theta) * sin(phi);
}

/// Convert spherical coordinates to cartesian 
inline void	spherical_to_cartesian ( const vector3& sph, vector3& cart )
{
	spherical_to_cartesian(sph.x(), sph.y(), sph.z(), cart); 
}

/// Solve quadratic equation
/// Returns false in case of no real roots exist
/// true otherwise
bool	SolveQuadratic( const real& a, const real& b, const real&c, real& x1, real& x2 );

/// Matrix transforms
matrix4x4 translation_matrix(vector3 const& v);
matrix4x4 rotation_matrix_x(real const& ang);
matrix4x4 rotation_matrix_y(real const& ang);
matrix4x4 rotation_matrix_z(real const& ang);
matrix4x4 rotation_matrix(vector3 const& axis, real const& ang);
matrix4x4 scale_matrix(vector3 const& v);

/// This perspective projection matrix effectively maps view frustum to [-1,1]x[-1,1]x[0,1] clip space, i.e. DirectX depth
matrix4x4 perspective_proj_matrix_lh_dx(real const& l, real const& r, real const& b, real const& t, real const& n, real const& f);
matrix4x4 perspective_proj_matrix_lh_gl(real const& l, real const& r, real const& b, real const& t, real const& n, real const& f);

/// This perspective projection matrix effectively maps view frustum to [-1,1]x[-1,1]x[-1,1] clip space, i.e. OpenGL depth
matrix4x4 perspective_proj_matrix_rh_gl(real const& l, real const& r, real const& b, real const& t, real const& n, real const& f);

matrix4x4 perspective_proj_fovy_matrix(real const& fovy, real const& aspect, real const& n, real const& f);
matrix4x4 perspective_proj_fovy_matrix_lh_dx(real const& fovy, real const& aspect, real const& n, real const& f);
matrix4x4 perspective_proj_fovy_matrix_lh_gl(real const& fovy, real const& aspect, real const& n, real const& f);
matrix4x4 perspective_proj_fovy_matrix_rh_gl(real const& fovy, real const& aspect, real const& n, real const& f);

matrix4x4 lookat_matrix_lh_dx( vector3 const& pos, vector3 const& at, vector3 const& up);

/// Quaternion transforms
quat	rotation_quat(vector3 const& axis, real const& angle);
vector3 rotate_vector( vector3 const& v, quat const& q );
quat	rotate_quat( quat const& v, quat const& q );

void LoadFileContents(std::string const& name, std::vector<char>& contents, bool binary = false);

#endif