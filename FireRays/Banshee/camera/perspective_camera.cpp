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
    right_   = cross(forward_, normalize(up));
    up_      = cross(right_, forward_);

    // Image plane parameters
    // tan(fovy/2) = (height / 2) / znear
    // height = 2 * tan(fovy/2) * znear
    // width = aspect * height
    dim_.x = 2 * tanf(fovy * 0.5f) * zcap_.x;
    dim_.y = dim_.x / aspect_;
}

void PerscpectiveCamera::GenerateRay(float2 const& sample, ray& r) const
{
    // Transform into [-0.5,0.5]
    float2 hsample = sample - float2(0.5f, 0.5f);
    // Transform into [-dim/2, dim/2]
    float2 csample = hsample * dim_;

    // Origin == camera position
    r.o = p_;
    // Direction to image plane
    r.d = normalize(zcap_.x * forward_ + csample.x * right_ + csample.y * up_);
    // Zcap == (znear,zfar)
    r.t = float2(zcap_.x, zcap_.y);
}