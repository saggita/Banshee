#ifndef HLBVH_H
#define HLBVH_H

#include <memory>
#include <vector>

#include "bvh.h"

///< The implementation of BVH based on spatial Z-curve sorting
///< https://research.nvidia.com/publication/hlbvh-hierarchical-lbvh-construction-real-time-ray-tracing
///<
class Hlbvh : public Bvh
{
public:
    Hlbvh()
    {}
    
protected:
    // Build function
    virtual void BuildImpl(std::vector<Primitive*> const& prims);
    // Find split based on Morton codes
    int FindSplit(std::vector<std::pair<int,unsigned int> > const& mortoncodes, int left, int right);
};


#endif // HLBVH_H
