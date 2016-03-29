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

#ifndef WORLD_H
#define WORLD_H

#include <memory>
#include <vector>

#include "../primitive/shapebundle.h"
#include "../accelerator/intersectable.h"
#include "../light/light.h"
#include "../camera/camera.h"
#include "../material/material.h"

///< World class is a container for all entities for the scene. 
///< It hosts entities and is in charge of destroying them.
///< For convenience reasons it impelements Primitive interface
///< to be able to provide intersection capabilities
///<
class World : public Intersectable
{
public:
    World()
        : accel_(nullptr)
        , camera_(nullptr)
        , bgcolor_(float3(0,0,0))
    {
    }

    virtual ~World(){}
    
    void Commit();
    
    /**
     Intersectable overrides
     */
    // Intersection test
    bool Intersect(ray const& r, ShapeBundle::Hit& hit) const;
    // Intersection check test
    bool Intersect(ray const& r) const;


public:
    // Lights
    std::vector<std::unique_ptr<Light> > lights_;
    // Objects in the scene
    std::vector<std::unique_ptr<ShapeBundle> > shapebundles_;
    // Accelerator
    std::unique_ptr<Intersectable> accel_;
    // Camera
    // TODO: account for multiple cameras
    std::unique_ptr<Camera> camera_;
    // Background color
    float3 bgcolor_;
    // Materials
    std::vector<std::unique_ptr<Material> > materials_;
};


#endif // WORLD_H