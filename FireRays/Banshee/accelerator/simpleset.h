#ifndef SIMPLESET_H
#define SIMPLESET_H

#include "../primitive/primitive.h"

#include <vector>
#include <memory>


///< SimpleSet is a very simple accelerator providing
///< O(N) intersection complexity. This accelerator is
///< not supposed to be used in production rendering and
///< really designed for testing/development needs
///<
class SimpleSet: public Primitive
{
public:
    SimpleSet()
    {}
    
    // Intersection test
    bool Intersect(ray& r, float& t, Intersection& isect) const;
    // Intersection check test
    bool Intersect(ray& r) const;
    // World space bounding box
    bbox Bounds() const;

    // Add primitive into the structure.
    // Note that the method takes over the ownership
    // of the primitive and will release the memory
    // in SimpleSet destructor
    void Emplace(Primitive* primitive);

private:
    SimpleSet(SimpleSet const&);
    SimpleSet& operator = (SimpleSet const&);
    
    // Vector of primitives owned by this instance
    // Note that a separate vector is used for intersection testing
    // since some of the primitives might not be intersectable
    // and they need to be refined in this case
    typedef std::vector<std::unique_ptr<Primitive> > PrimitiveStorageArray;
    PrimitiveStorageArray primitiveStorage_;
    typedef std::vector<Primitive*> PrimitiveArray;
    PrimitiveArray primitives_;

    // Bounding box of the overall structure
    bbox bounds_;
};


#endif // SIMPLESET_H
