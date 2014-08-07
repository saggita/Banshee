#ifndef WORLD_H
#define WORLD_H

#include <memory>
#include <vector>

#include "../primitive/primitive.h"
#include "../light/light.h"
#include "../camera/camera.h"

///< World class is a container for all entities for the scene. 
///< It hosts entities and is in charge of destroying them.
///< For convenience reasons it impelements Primitive interface
///< to be able to provide intersection capabilities
///<
class World : public Primitive
{
public:
    World()
        : accelerator_(nullptr)
        , camera_(nullptr)
        , bgcolor_(float3(0,0,0))
    {
    }

    virtual ~World(){}

    // Intersection test
    bool Intersect(ray& r, float& t, Intersection& isect) const;
    // Intersection check test
    bool Intersect(ray& r) const;
    // World space bounding box
    bbox Bounds() const;

public:
    // Lights
    std::vector<std::unique_ptr<Light> > lights_;
    // Primitive representing an acceleration structure
    std::unique_ptr<Primitive> accelerator_;
    // Camera
    // TODO: account for multiple cameras
    std::unique_ptr<Camera> camera_;
    // Background color
    float3 bgcolor_;
};


#endif // WORLD_H