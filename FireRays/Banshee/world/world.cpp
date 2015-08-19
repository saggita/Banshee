#include "world.h"
#include "../accelerator/bvh.h"
#include "../accelerator/embree.h"

#define EMBREE

void World::Commit()
{
#ifndef EMBREE
    Bvh* bvh = new Bvh(true);
    bvh->Build(shapebundles_);
    accel_.reset(bvh);
#else
    Embree* embree = new Embree();
    embree->Build(shapebundles_);
    accel_.reset(embree);
#endif
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