#include "environment_camera.h"

#include "../math/mathutils.h"

#include <cmath>

EnvironmentCamera::EnvironmentCamera(float3 const& eye, float3 const& at, float3 const& up, float2 const& zcap)
: p_(eye)
, zcap_(zcap)
{
    // Construct camera frame
    forward_ = normalize(at - eye);
    right_   = cross(normalize(up), forward_);
    up_      = cross(forward_, right_);

    
}

void EnvironmentCamera::GenerateRay(float2 const& sample, ray& r) const
{
    // We are measuring spherical angles in the following fashion here:
    // thetha from 0 to PI from positive y direction (up)
    // phi from 0 to 2*PI from x(right) axis to z(forward) axis (counter-clockwise)
    //
    // We need the following mapping to get positive z direction (forward)
    // to point into the center of the image plane(0.5, 0.5):
    // img samle x-> phi
    // 0 -> -PI/2
    // 0.5 -> PI/2
    // 1 -> 3 * PI/2
    // img sample y->theta
    // 0 -> PI
    // 0.5->PI/2
    // 1->0
    // => formulae
    // phi new = phi - PI/2
    // thetha new = PI - thetha
    float phi = sample.x * 2 * PI - PI * 0.5f;
    float thetha = PI - sample.y * PI;
    
    // Origin == camera position
    r.o = p_;
    // Direction to image plane
    r.d = right_ * sinf(thetha) * cosf(phi) + forward_ * sinf(thetha) * sinf(phi) + up_ * cosf(thetha);
    // Zcap == (znear,zfar)
    r.t = float2(zcap_.x, zcap_.y);
}
