#ifndef WORLD_H
#define WORLD_H

#include <memory>
#include <vector>

#include "../primitive/primitive.h"
#include "../light/light.h"

///< World class is a container for all entities for the scene. 
///< It hosts entities and is in charge of destroying them.
///< For convenience reasons it impelements Primitive interface
///< to be able to provide intersection capabilities
class World : public Primitive
{
public:
    virtual ~World(){}
    
    // Intersection test
    bool Intersect(ray& r, float& t, Intersection& isect) const;
    // Intersection check test
    bool Intersect(ray& r) const;
    // World space bounding box
    bbox Bounds() const;

public:
    // Lights
    std::vector<Light> lights_;
    // Primitive representing an acceleration structure
    std::unique_ptr<Primitive> accelerator_;
};


#endif // WORLD_H