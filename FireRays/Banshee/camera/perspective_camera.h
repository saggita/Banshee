#ifndef PERSPECTIVE_CAMERA_H
#define PERSPECTIVE_CAMERA_H

#include "camera.h"
#include "../math/float3.h"

class PerscpectiveCamera: public Camera
{
public:
    PerscpectiveCamera(float3 const& eye, float3 const& at, float3 const& up, 
                       float2 const& zcap, float fovy, float aspect);

    void GenerateRay(float2 const& sample, ray& r) const;

private:
    // Camera coordinate frame
    float3 forward_;
    float3 right_;
    float3 up_;
    float3 p_;

    // Near and far Z
    float2 zcap_;
    float  fovy_;
    float  aspect_;

    // Image plane height & width
    float2 dim_;
};

#endif // PERSPECTIVE_CAMERA_H