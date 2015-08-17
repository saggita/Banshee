#include "world.h"
#include "../accelerator/bvh.h"



void World::Commit()
{
    Bvh* bvh = new Bvh(true);
    bvh->Build(shapebundles_);
    accel_.reset(bvh);
}

// Intersection test
bool World::Intersect(ray const& r, ShapeBundle::Hit& hit) const
{
    hit.t = r.t.y;
    return accel_->Intersect(r, hit);
}

// Intersection check test
bool World::Intersect(ray const& r) const
{
    return accel_->Intersect(r);
}