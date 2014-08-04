#include "world.h"

bool World::Intersect(ray& r, float& t, Intersection& isect) const
{
    return accelerator_->Intersect(r, t, isect);
}

bool World::Intersect(ray& r) const
{
    return accelerator_->Intersect(r);
}

bbox World::Bounds() const
{
    return accelerator_->Bounds();
}