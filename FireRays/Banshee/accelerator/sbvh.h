#ifndef SBVH_H
#define SBVH_H

#include "bvh.h"

///< BVH implementation which is based on SAH object/space splits 
///< Link: http://www.nvidia.com/object/nvidia_research_pub_012.html
///<
class Sbvh : public Bvh
{
public:
    // Pass triangle intersection cost compared to unit node traversal cost
    // 0 - no cost (intersection is free)
    // FLT_MAX - maximum cost (any intersection is heavier than any traversal)
    Sbvh(float trisah, int maxleafprims = 32,
         bool usespatial = true,
         int maxsptialdepth = 20,
         float minoverlap = 0.00001)
        : trisah_(trisah)
        , maxleafprims_(maxleafprims)
        , usespatial_(usespatial)
        , maxspatialdepth_(maxsptialdepth)
        , minoverlap_(minoverlap)
    {
    }

protected:
    // Build function
    void BuildImpl(std::vector<Primitive*> const& prims);

private:
    // Struct to describe reference to a primitive
    struct PrimitiveRef
    {
        bbox bounds;
        int  idx;
    };

    // Struct to describe a split
    struct Split
    {
        int dim;
        float sah;
        float border;
        float overlaparea;
    };

    // Find best object split based on SAH
    Split FindObjectSplit(std::vector<PrimitiveRef> const& primrefs, int startidx, int numprims, bbox const& bounds, bbox const& centroid_bounds) const;
    // Find best spatial split based on SAH
    Split FindSpatialSplit(std::vector<PrimitiveRef> const& primrefs, int startidx, int numprims, bbox const& bounds, int depth) const;
    // Perform object split
    int   PerformObjectSplit(Split const& split, std::vector<PrimitiveRef>& primrefs, int startidx, int numprims) const;
    // Split primitive reference into 2 parts
    bool  SplitPrimRef(PrimitiveRef const& ref, int axis, float border, PrimitiveRef& leftref, PrimitiveRef& rightref) const;
    // Perform spatial split
    void  PerformSpatialSplit(Split const& split, std::vector<PrimitiveRef>& primrefs, int startidx, int numprims, int& idx, int& newnumprims) const;

    // Triangle intersection cost as a fraction of node traversal cost
    float trisah_;
    // Whether use spatial splits or not
    bool usespatial_;
    // Maximum allowed number of primitives in the leaf
    int maxleafprims_;
    // Maximum depth at which spatial split is allowed
    int maxspatialdepth_;
    // Minimum children overlap to consider a spatial split
    float minoverlap_;
};

#endif //SBVH_H