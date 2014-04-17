#ifndef VECTOR3OPT_H
#define VECTOR3OPT_H

#include <xmmintrin.h>
#include <smmintrin.h>

#if defined(_MSC_VER)
#define _ALIGNED_STRUCT(x) __declspec(align(x)) struct
#define _ALIGNED_CLASS(x) __declspec(align(x)) class
#else
#define _ALIGNED_STRUCT(x) struct __attribute__ ((aligned(x)))
#define _ALIGNED_CLASS(x) class __attribute__ ((aligned(x)))
#endif
 
// Simple vector class
_ALIGNED_CLASS(16) vector3opt
{
 public:
  // constructors
  inline vector3opt() : mmvalue(_mm_setzero_ps()) {}
  inline vector3opt(float x, float y, float z) : mmvalue(_mm_set_ps(0, z, y, x)) {}
  inline vector3opt(__m128 m) : mmvalue(m) {}
 
  // arithmetic operators with vector3opt
  inline vector3opt operator+(const vector3opt& b) const { return _mm_add_ps(mmvalue, b.mmvalue); }
  inline vector3opt operator-(const vector3opt& b) const { return _mm_sub_ps(mmvalue, b.mmvalue); }
  inline vector3opt operator*(const vector3opt& b) const { return _mm_mul_ps(mmvalue, b.mmvalue); }
  inline vector3opt operator/(const vector3opt& b) const { return _mm_div_ps(mmvalue, b.mmvalue); }
 
  // op= operators
  inline vector3opt& operator+=(const vector3opt& b) { mmvalue = _mm_add_ps(mmvalue, b.mmvalue); return *this; }
  inline vector3opt& operator-=(const vector3opt& b) { mmvalue = _mm_sub_ps(mmvalue, b.mmvalue); return *this; }
  inline vector3opt& operator*=(const vector3opt& b) { mmvalue = _mm_mul_ps(mmvalue, b.mmvalue); return *this; }
  inline vector3opt& operator/=(const vector3opt& b) { mmvalue = _mm_div_ps(mmvalue, b.mmvalue); return *this; }
 
  // arithmetic operators with float
  inline vector3opt operator+(float b) const { return _mm_add_ps(mmvalue, _mm_set1_ps(b)); }
  inline vector3opt operator-(float b) const { return _mm_sub_ps(mmvalue, _mm_set1_ps(b)); }
  inline vector3opt operator*(float b) const { return _mm_mul_ps(mmvalue, _mm_set1_ps(b)); }
  inline vector3opt operator/(float b) const { return _mm_div_ps(mmvalue, _mm_set1_ps(b)); }
 
  // op= operators with float
  inline vector3opt& operator+=(float b) { mmvalue = _mm_add_ps(mmvalue, _mm_set1_ps(b)); return *this; }
  inline vector3opt& operator-=(float b) { mmvalue = _mm_sub_ps(mmvalue, _mm_set1_ps(b)); return *this; }
  inline vector3opt& operator*=(float b) { mmvalue = _mm_mul_ps(mmvalue, _mm_set1_ps(b)); return *this; }
  inline vector3opt& operator/=(float b) { mmvalue = _mm_div_ps(mmvalue, _mm_set1_ps(b)); return *this; }
 
  // cross product
  inline vector3opt cross(const vector3opt& b) const
  {
   return _mm_sub_ps(
    _mm_mul_ps(_mm_shuffle_ps(mmvalue, mmvalue, _MM_SHUFFLE(3, 0, 2, 1)), _mm_shuffle_ps(b.mmvalue, b.mmvalue, _MM_SHUFFLE(3, 1, 0, 2))), 
    _mm_mul_ps(_mm_shuffle_ps(mmvalue, mmvalue, _MM_SHUFFLE(3, 1, 0, 2)), _mm_shuffle_ps(b.mmvalue, b.mmvalue, _MM_SHUFFLE(3, 0, 2, 1)))
   );
  }
 
  // dot product with another vector
  inline float dot(const vector3opt& b) const { return _mm_cvtss_f32(_mm_dp_ps(mmvalue, b.mmvalue, 0x71)); }
  // length of the vector
  inline float length() const { return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(mmvalue, mmvalue, 0x71))); }
  // 1/length() of the vector
  inline float rlength() const { return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_dp_ps(mmvalue, mmvalue, 0x71))); }
  // returns the vector scaled to unit length
  inline vector3opt normalize() const { return _mm_mul_ps(mmvalue, _mm_rsqrt_ps(_mm_dp_ps(mmvalue, mmvalue, 0x7F))); }
 
  // overloaded operators that ensure alignment
  inline void* operator new[](size_t x) { return _mm_malloc(x, 16); }
  inline void  operator delete[](void* x) { if (x) _mm_free(x); }
  inline float operator [](int i) const { return *(&x + i);}
 
  // Member variables
  union
  {
   struct { float x, y, z; };
   __m128 mmvalue;
  };
};
 
inline vector3opt operator+(float a, const vector3opt& b) { return b + a; }
inline vector3opt operator-(float a, const vector3opt& b) { return vector3opt(_mm_set1_ps(a)) - b; }
inline vector3opt operator*(float a, const vector3opt& b) { return b * a; }
inline vector3opt operator/(float a, const vector3opt& b) { return vector3opt(_mm_set1_ps(a)) / b; }

inline vector3opt cross(const vector3opt& a, const vector3opt& b) { return a.cross(b); }
inline float dot(const vector3opt& a, const vector3opt& b) { return a.dot(b); }
inline vector3opt normalize(const vector3opt& a) { return a.normalize(); }

inline vector3opt vmin(const vector3opt& a, const vector3opt& b)
{
    return vector3opt(_mm_min_ps(a.mmvalue, b.mmvalue));
}

inline vector3opt vmax(const vector3opt& a, const vector3opt& b)
{
    return vector3opt(_mm_max_ps(a.mmvalue, b.mmvalue));
}

#endif // VECTOR3OPT