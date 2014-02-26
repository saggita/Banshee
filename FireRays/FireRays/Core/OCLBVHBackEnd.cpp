//
//  OCLBVHBackEnd.cpp
//  FireRays
//
//  Created by dmitryk on 25.02.14.
//
//

#include "OCLBVHBackEnd.h"


OCLBVHBackEnd::OCLBVHBackEnd(BVH& bvh)
: bvh_(bvh)
{
    
}

void OCLBVHBackEnd::Generate()
{
    nodes_.clear();
    
    std::map<BVH::NodeId, unsigned> indices;
    std::vector<BVH::NodeId>  rightChildren;
    
    for (BVH::Iterator* dfi = bvh_.CreateDepthFirstIterator(); dfi->HasNext(); dfi->Next())
    {
        
        Node newNode;
        BVH::NodeId rightChild;
        newNode.box = bvh_.GetNodeBbox(dfi->GetNodeId());
        
        switch (bvh_.GetNodeType(dfi->GetNodeId())) {
            case BVH::NodeType::NT_INTERNAL:
                newNode.primCount = 0;
                newNode.primStartIdx = (unsigned)bvh_.GetNodeSplitAxis(dfi->GetNodeId());
                newNode.parent = 0;
                rightChild = bvh_.GetRightChild(dfi->GetNodeId());
                break;
                
            case BVH::NodeType::NT_LEAF:
                bvh_.GetLeafPrimitives(dfi->GetNodeId(), newNode.primStartIdx, newNode.primCount);
                newNode.parent = 0;
                rightChild = nullptr;
                break;
        }
        
        nodes_.push_back(newNode);
        rightChildren.push_back(rightChild);
        indices[dfi->GetNodeId()] = nodes_.size() - 1;
    }
    
    for(int i = 0; i < nodes_.size(); ++i)
    {
        nodes_[i].right = nodes_[i].primCount == 0 ? (indices[rightChildren[i]]) : 0;
    }
}

OCLBVHBackEnd::Node const*  OCLBVHBackEnd::GetNodes() const
{
    return &nodes_[0];
}

unsigned int OCLBVHBackEnd::GetNodeCount() const
{
    return nodes_.size();
}
