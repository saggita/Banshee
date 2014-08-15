#ifndef PERSPECTIVE_CAMERA_H
#define PERSPECTIVE_CAMERA_H

#include "camera.h"
#include "../math/float3.h"


///< PerspectiveCamera is a class defining perspective viewing system.
///< The camera has field of view and image plane aspect ration settings.
///< The wider the FOV is the more perspective distortion you get,
///< but larger portion of the scene is visible
///<
class PerscpectiveCamera: public Camera
{
public:
    // Pass camera position, camera aim, camera up vector, depth limits, vertical field of view
    // and image plane aspect ratio
    PerscpectiveCamera(float3 const& eye, float3 const& at, float3 const& up, 
                       float2 const& zcap, float fovy, float aspect);

    ///< sample is a value in [0,1] square describing where to sample the image plane
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

    // Image plane width & hight in scene units
    float2 dim_;
};

#endif // PERSPECTIVE_CAMERA_H