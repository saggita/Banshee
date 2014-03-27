//
//  BVH.h
//  BVHOQ
//
//  Created by dmitryk on 09.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#ifndef BVH_H
#define BVH_H

#include "Common.h"
#include "bbox.h"
#include "SceneBase.h"

class BVH
{
public:
    BVH();
    ~BVH();
    
    /// TYPES
    // Common node identifier, can be cast to Node* iternally
    typedef void* NodeId;
    
    // Node type
    enum class NodeType
    {
        NT_INTERNAL,
        NT_LEAF
    };
    
    // Split axis
    enum class SplitAxis
    {
        SA_X,
        SA_Y,
        SA_Z
    };

    // Iterator interface
    struct Iterator;
    
    // TRAVERSAL INTERFACE
    // Breadth and depth first traversals are supported
    Iterator*   CreateBreadthFirstIterator();
    Iterator*   CreateDepthFirstIterator();

    // You need to destroy your iterator after you have done with it
    void        DestroyIterator(Iterator* );
    

    /// NODE DATA ACCESS
    // Node data access methods
    BBox const& GetNodeBbox(NodeId id) const;
    NodeType    GetNodeType(NodeId id) const;
    SplitAxis   GetNodeSplitAxis(NodeId id) const;
    unsigned    GetLeafPrimitiveCount(NodeId id) const;
    void        GetLeafPrimitives(NodeId id, std::vector<unsigned>& v) const;
    void        GetLeafPrimitives(NodeId id, unsigned* prims) const;
    void        GetLeafPrimitives(NodeId id, unsigned& primStartIdx, unsigned& primCount) const;
    NodeId      GetRightChild(NodeId id) const;
    NodeId      GetLeftChild(NodeId id) const;
    
    /// BVH DATA ACCESS
    unsigned const*   GetPrimitiveIndices() const;
    unsigned          GetPrimitiveIndexCount() const;

    /// QUALITY CHECK : raycast query
    struct RayQuery;
    struct RayQueryStatistics;
    void        CastRay(RayQuery& q, RayQueryStatistics& stat, SceneBase::Vertex const* vertices, unsigned const* indices) const;
    // Node count
    unsigned GetNodeCount() const;
    
protected:
    // Interface for the builders, make them friends here
    friend class BVHAccelerator;
    friend class SplitBVHBuilder;
    friend class LinearBVHBuilder;
    // Iterators are also friends
    friend class DepthFirstIterator;
    friend class BreadthFirstIterator;
    // Relation to parent: right or left child
    // left child has lesser coordinate values along the split axis
    enum ChildRel
    {
        CR_LEFT,
        CR_RIGHT
    };
    
    // Main builder interface
    // Get the id preceeding to root node, this is needed to initialize root
    NodeId GetPreRootNode() const;
    // Create internal node and attach to the node identified by id to the side
    // identified by rel (left or right)
    NodeId CreateInternalNode(NodeId id, ChildRel rel, SplitAxis axis, BBox const& b);
    // Create leaf node and attach along with a list of primitive indices
    NodeId CreateLeafNode(NodeId id, ChildRel rel, BBox const& b, unsigned* primIndices, unsigned idxCount);
    // Check whether node childrens' bboxes are contained within its own bounding box
    bool   CheckInvariant(NodeId id);
    


private:
    BVH(BVH const&);
    BVH& operator = (BVH const&);

    // Node descriptor
    struct Node;
    
    // Root node
    Node*                   root_;
    // Node cache to check an eistence, should be removed later on
    std::set<NodeId>        nodeCache_;
    // Packed primitive indices, leafs reference prim indices in this array
    // by specifying start index and number of primitives in the leaf
    std::vector<unsigned>   primIndices_;
};

// Iterator interface
struct BVH::Iterator
{
    virtual ~Iterator(){}
    virtual NodeId GetNodeId() = 0;
    virtual void   Reset() = 0;
    virtual bool   HasNext() = 0;
    virtual void   Next() = 0;
};

struct BVH::RayQuery
{
    vector3 o;
    vector3 d;
    float   t;
};


struct BVH::RayQueryStatistics
{
    bool            hitBvh;
    bool            hitPrim;
    unsigned int    numNodesVisited;
    unsigned int    numTrianglesTested;
    unsigned int    maxDepthVisited;
};

inline BVH::NodeId BVH::GetPreRootNode() const
{
    return nullptr;
}

inline unsigned BVH::GetNodeCount() const
{
    return nodeCache_.size();
}





#endif