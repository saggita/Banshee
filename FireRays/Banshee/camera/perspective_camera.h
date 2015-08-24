/*
    Banshee and all code, documentation, and other materials contained
    therein are:

        Copyright 2013 Dmitry Kozlov
        All Rights Reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:
        * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
        * Neither the name of the software's owners nor the names of its
        contributors may be used to endorse or promote products derived from
        this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
    (This is the Modified BSD License)
*/
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

protected:
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