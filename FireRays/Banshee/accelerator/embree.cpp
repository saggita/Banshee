#include "embree.h"

#include <embree2/rtcore.h>

Embree::Embree()
{
    rtcInit();
}

Embree::~Embree()
{
    rtcExit();
}

// Build function: pass bounding boxes and
void Embree::Build(std::vector<Primitive*> const& prims)
{
    
}

// Intersection test
bool Embree::Intersect(ray& r, float& t, Intersection& isect) const
{
    
}

// Intersection check test
bool Embree::Intersect(ray& r) const
{
    
}