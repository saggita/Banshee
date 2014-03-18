//
//  SplitBVHBuilder.h
//  FireRays
//
//  Created by dmitryk on 21.02.14.
//
//

#ifndef SPLITBVHBUILDER_H
#define SPLITBVHBUILDER_H

#include "BVH.h"
#include "BVHBuilderBase.h"

class SplitBVHBuilder : public BVHBuilderBase
{
public:
    struct Primitive;
    
    // Construct BVH from an indexed vertex list
    template <typename T> SplitBVHBuilder(T const* vertices, unsigned int vertexCount, unsigned const* indices, unsigned indexCount, unsigned const* materials, unsigned primsPerLeaf, unsigned minPrimsPerLeaf, float triSahCost, float nodeSahCost);
    
    // Start building process
    void Build();
    
    unsigned         GetPrimitiveCount() const;
    Primitive const* GetPrimitives() const;
    
    
private:
    // Reference to a primitive
    struct PrimitiveRef;
    struct SplitDesc;
    struct NodeDesc;
    
    typedef std::list<PrimitiveRef>::iterator PrimitiveRefIterator;
    
    // Build node from [first, last) prim refs range and attach it to parent from rel-side
    BVH::NodeId BuildNode(BVH::NodeId parentNode, BVH::ChildRel rel, PrimitiveRefIterator first, PrimitiveRefIterator last, unsigned level);
    
    // Find best object split according to 64-bins histogram SAH
    void FindObjectSplit(NodeDesc const& desc, std::vector<PrimitiveRef>::iterator begin, std::vector<PrimitiveRef>::iterator end, SplitDesc& split);
    
    // Find best spatial split according to 64-bins histogram SAH
    void FindSpatialSplit(NodeDesc const& desc, std::vector<PrimitiveRef>::iterator begin, std::vector<PrimitiveRef>::iterator end, SplitDesc& split);

    // Perform object split
    std::vector<PrimitiveRef>::iterator PerformObjectSplit(std::vector<PrimitiveRef>::iterator begin, std::vector<PrimitiveRef>::iterator end, SplitDesc const& splitDesc);
    
    // Perform spatial split
    std::vector<PrimitiveRef>::iterator PerformSpatialSplit(std::vector<PrimitiveRef>::iterator begin, std::vector<PrimitiveRef>::iterator primsEnd, SplitDesc const& splitDesc, unsigned& newPrimCount);
    
    // Create node desc for the range
    void CreateNodeDesc(std::vector<PrimitiveRef>::iterator begin, std::vector<PrimitiveRef>::iterator end, NodeDesc& desc);

    // Test if the prim ref intersects the plane and return adjusted primitve refs for right and left
    bool SplitPrimRef(PrimitiveRef primRef, int splitAxis, float splitValue, PrimitiveRef& r1, PrimitiveRef& r2);
    
    // Reorder primitive array to conform to BVH representation
    void ReorderPrimitives();
    
    PrimitiveRefIterator RemoveEmptyRefs(PrimitiveRefIterator begin, PrimitiveRefIterator end);
    
    
    // Data members
    std::vector<Primitive>  prims_;
    std::list<PrimitiveRef> refs_;
    std::vector<vector3>    positions_;
    float                   triSahCost_;
    float                   nodeSahCost_;
    unsigned                primsPerLeaf_;
    unsigned                minPrimsPerLeaf_;
    unsigned                maxLevel_;
};


struct SplitBVHBuilder::PrimitiveRef
{
    unsigned idx;
    BBox     bbox;
};

struct SplitBVHBuilder::SplitDesc
{
    unsigned dim;
    float    val;
    float    sah;
    
    BBox lb;
    BBox rb;
};

struct SplitBVHBuilder::NodeDesc
{
    std::vector<PrimitiveRef>::iterator begin;
    std::vector<PrimitiveRef>::iterator end;
    BBox     bbox;
    BBox     cbox;
};

struct SplitBVHBuilder::Primitive
{
    union
    {
        struct
        {
            unsigned i1,i2,i3,m;
        };
        
        unsigned i[4];
    };
};

template <typename T> SplitBVHBuilder::SplitBVHBuilder(T const* vertices, unsigned int vertexCount, unsigned const* indices, unsigned indexCount, unsigned const* materials, unsigned primsPerLeaf, unsigned minPrimsPerLeaf, float triSahCost, float nodeSahCost)
: primsPerLeaf_(primsPerLeaf)
, minPrimsPerLeaf_(minPrimsPerLeaf)
, triSahCost_(triSahCost)
, nodeSahCost_(nodeSahCost)
, maxLevel_(0)
{
    positions_.resize(vertexCount);
    
    /// copy vertex data
    for (int i = 0; i < vertexCount; ++i)
    {
        positions_[i] = vertices[i].position;
    }
    
    /// build primitives list
    prims_.resize(indexCount/3);
    for (unsigned i = 0; i < indexCount; i += 3)
    {
        BBox b(vertices[indices[i]].position, vertices[indices[i+1]].position);
        b = BBoxUnion(b, vertices[indices[i+2]].position);
        
        Primitive t = {indices[i], indices[i + 1], indices[i + 2], materials[i/3]};
        prims_[i/3] = t;
        
        PrimitiveRef info = {i/3, b};
        refs_.push_back(info);
    }
}

#endif //SPLITBVHBUILDER_H
