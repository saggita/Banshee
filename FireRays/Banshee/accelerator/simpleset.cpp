#include "simpleset.h"


bool SimpleSet::Intersect(ray& r, float& t, Intersection& isect) const
{
    bool found = false;
    
    // Go thru all of the primitives and adjust ray intersection range on every primitive
    for (PrimitiveArray::const_iterator i = primitives_.begin(); i != primitives_.end(); ++i)
    {
        if ((*i)->Intersect(r, t, isect))
        {
            found = true;
            r.t.y = t;
        }
    }
    
    return found;
}

bool SimpleSet::Intersect(ray& r) const
{
    // Go thru all of the primitives and return on first intersection
    for (PrimitiveArray::const_iterator i = primitives_.begin(); i != primitives_.end(); ++i)
    {
        if ((*i)->Intersect(r))
        {
            return true;
        }
    }
    
    return false;
}

bbox SimpleSet::Bounds() const
{
    // Return cached result
    return bounds_;
}

void SimpleSet::Emplace(Primitive* primitive)
{
    // Emplace primitive
    primitives_.push_back(std::unique_ptr<Primitive>(primitive));
    // Recalculate bounds
    bounds_ = bboxunion(bounds_, primitive->Bounds());
}
