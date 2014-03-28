#ifndef common_types_H
#define common_types_H

// Util classes
#define _USE_MATH_DEFINES
#include <math.h>
#include "matrix.h"
#include "vector.h"
#include "quaternion.h"
#include "vector3opt.h"

// Floating point type to use within core library
typedef float		real;
typedef unsigned int  uint;
typedef unsigned char byte;

// Vector types
typedef vector<real, 1> vector1;
typedef vector<real, 2> vector2;
typedef vector<real, 3> vector3;
typedef vector<real, 4> vector4;

// Color types
typedef vector3 color_rgb;
typedef vector4 color_rgba;

// Matrix types
typedef matrix<real, 2, 2> matrix2x2;
typedef matrix<real, 3, 3> matrix3x3;
typedef matrix<real, 4, 4> matrix4x4;

// Quaternion type
typedef quaternion<real> quat;

// Rect
template <typename T> struct rect
{
	T x,y;
	T w,h;
	rect( T const& xx = T(), T const& yy = T(), T const& ww = T(), T const& hh = T() ) :
	x(xx), y(yy), w(ww), h(hh) {}
};

typedef rect<real> r_rect;
typedef rect<unsigned int> ui_rect;

// Size
template <typename T> struct size
{
	T w,h;
	size( T const& ww = T(), T const& hh = T() ) :
	w(ww), h(hh) {}
};

typedef size<real> r_size;
typedef size<uint> ui_size;

struct frustum
{
	real fovy;
	real aspect;
	real nr;
	real fr;

	frustum(real fv = real(), real a = real(), real n = real(), real f = real())
		: fovy(fv)
		,aspect(a)
		,nr(n)
		,fr(f)
	{
	}
};

struct ray
{
	vector3 o;
	vector3 d;
	
	ray(vector3 origin, vector3 dir) : o(origin), d(dir){}
};

#endif