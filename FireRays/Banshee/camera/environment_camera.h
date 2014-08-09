#ifndef ENVIRONMENT_CAMERA_H
#define ENVIRONMENT_CAMERA_H

#include "camera.h"
#include "../math/float3.h"

class EnvironmentCamera: public Camera
{
public:
    EnvironmentCamera(float3 const& eye, float3 const& at, float3 const& up, float2 const& zcap);
    
    void GenerateRay(float2 const& sample, ray& r) const;
    
private:
    // Camera frame
    float3 p_;
    float3 forward_;
    float3 right_;
    float3 up_;
    
    // Near and far Z
    float2 zcap_;
};

#endif // ENVIRONMENT_CAMERA_H
