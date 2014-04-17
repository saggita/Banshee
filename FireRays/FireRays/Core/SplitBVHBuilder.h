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

#include "aligned_allocator.h"

class SplitBVHBuilder : public BVHBuilderBase
{
public:
    // Construct BVH from an indexed vertex list.
    // Position data gets copied into internal storage and doesn't need to be kept around
    // as well as index data.
    template <typename T> SplitBVHBuilder(T const* vertices, unsigned int vertexCount, unsigned const* indices, unsigned indexCount, unsigned const* materials, unsigned primsPerLeaf, float triSahCost, float nodeSahCost);
    
    // Start building process
    void Build();
    
    // Accessors to internal index data
    unsigned         GetPrimitiveCount() const;
    Primitive const* GetPrimitives() const;
    
private:

    // Reference to a primitive consists of (possibly partial)bounding box and primitive index om prims_ array 
    struct PrimitiveRef;
    typedef std::vector<PrimitiveRef, aligned_allocator<PrimitiveRef, 16> > PrimitiveRefVector;
    typedef std::list<PrimitiveRef, aligned_allocator<PrimitiveRef, 16> >   PrimitiveRefList;

    // Split is represented by dimension, split value and SAH cost value.
    // The same structure is used to represent spatial and object splits
    // split value == NaN is a special sentinel representing a command to
    // split in half in case histogram binning cannot be applied
    struct SplitDesc;

    // Node desc is represented by start iterator, end iterator and bounding box
    struct NodeDesc;
    
    typedef std::list<PrimitiveRef>::iterator PrimitiveRefIterator;
    
    // Build node from [first, last) prim refs range and attach it to parent from rel-side
    void BuildBVH();
    
    // Find best object split according to 64-bins histogram SAH
    void FindObjectSplit(NodeDesc const& desc, SplitDesc& split);
    
    // Find best spatial split according to 64-bins histogram SAH
    void FindSpatialSplit(NodeDesc const& desc, SplitDesc& split);

    // Perform object split
    std::vector<PrimitiveRef>::iterator PerformObjectSplit(std::vector<PrimitiveRef>::iterator begin, std::vector<PrimitiveRef>::iterator end, SplitDesc const& splitDesc);
    
    // Perform spatial split
    std::vector<PrimitiveRef>::iterator PerformSpatialSplit(std::vector<PrimitiveRef>::iterator begin, std::vector<PrimitiveRef>::iterator end, SplitDesc const& splitDesc, unsigned& newPrimCount);
    
    // Create node desc for the range of primitive refs
    void CreateNodeDesc(PrimitiveRefVector::iterator begin, std::vector<PrimitiveRef>::iterator end, NodeDesc& desc);

    // Test if the prim ref intersects the plane and return adjusted primitve refs for right and left
    bool SplitPrimRef(PrimitiveRef const& primRef, int splitAxis, float splitValue, PrimitiveRef& r1, PrimitiveRef& r2);
    
    // Reorder primitive array to conform to BVH representation
    void ReorderPrimitives();
    
    // Remove refs with zero-volume bounding boxes
    void RemoveEmptyRefs();

    // Data members
    std::vector<Primitive>  prims_;
    PrimitiveRefList refs_;
    std::vector<vector3opt, aligned_allocator<vector3opt, 16> > positions_;

    float                   triSahCost_;
    float                   nodeSahCost_;
    unsigned                primsPerLeaf_;
    unsigned                minPrimsPerLeaf_;
    unsigned                maxLevel_;
    unsigned                objectSplitCount_;
    unsigned                spatialSplitCount_;
};


_ALIGNED_STRUCT(16) SplitBVHBuilder::PrimitiveRef
{
    unsigned idx;
    BBox     bbox;
};

struct SplitBVHBuilder::SplitDesc
{
    unsigned dim;
    float    val;
    float    sah;
};

_ALIGNED_STRUCT(16) SplitBVHBuilder::NodeDesc
{
    PrimitiveRefVector::iterator begin;
    PrimitiveRefVector::iterator end;
    BBox     bbox;
    BBox     cbox;
};

template <typename T> SplitBVHBuilder::SplitBVHBuilder(T const* vertices, unsigned int vertexCount, unsigned const* indices, unsigned indexCount, unsigned const* materials, unsigned primsPerLeaf, float triSahCost, float nodeSahCost)
: primsPerLeaf_(primsPerLeaf)
, triSahCost_(triSahCost)
, nodeSahCost_(nodeSahCost)
, maxLevel_(0)
, objectSplitCount_(0)
, spatialSplitCount_(0)
{
    positions_.resize(vertexCount);
    
    /// copy vertex data
    for (int i = 0; i < vertexCount; ++i)
    {
        positions_[i] = vector3opt(vertices[i].position.x(), vertices[i].position.y(), vertices[i].position.z()); ;
    }
    
    /// build primitives list
    prims_.resize(indexCount/3);
    for (unsigned i = 0; i < indexCount; i += 3)
    {
        BBox b(positions_[indices[i]], positions_[indices[i+1]]);
        b = BBoxUnion(b, positions_[indices[i+2]]);
        
        Primitive t = {indices[i], indices[i + 1], indices[i + 2], materials[i/3]};
        prims_[i/3] = t;
        
        PrimitiveRef info = {i/3, b};
        refs_.push_back(info);
    }
}

#endif //SPLITBVHBUILDER_H
