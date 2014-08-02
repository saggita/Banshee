#include "perspective_camera.h"

#include <cmath>

PerscpectiveCamera::PerscpectiveCamera(float3 const& eye, float3 const& at, float3 const& up, 
                       float2 const& zcap, float fovy, float aspect) 
                       : p_(eye)
                       , zcap_(zcap)
                       , fovy_(fovy)
                       , aspect_(aspect)
{
    // Construct camera frame
    forward_ = normalize(at - eye);
    right_   = cross(normalize(up), forward_);
    up_      = cross(forward_, right_);

    // Image plane parameters
    // tan(fovy/2) = (height / 2) / znear
    // height = 2 * tan(fovy/2) * znear
    // width = aspect * height
    dim_.x = 2 * tanf(fovy * 0.5f) * zcap_.x;
    dim_.y = dim_.x * aspect_;
}

void PerscpectiveCamera::GenerateRay(float2 const& sample, ray& r) const
{
    float2 hsample = sample - float2(0.5f, 0.5f);
    float2 csample = hsample * dim_;

    r.o = p_;
    r.d = normalize(zcap_.x * forward_ + csample.x * right_ + csample.y * up_);
    r.t = float2(zcap_.x, zcap_.y);
}