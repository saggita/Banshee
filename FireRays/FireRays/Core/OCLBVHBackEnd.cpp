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
    
    for (BVH::Iterator* dfi = bvh_.CreateDepthFirstIterator();; dfi->Next())
    {
        Node newNode;
        BVH::NodeId rightChild;
        newNode.box = bvh_.GetNodeBbox(dfi->GetNodeId());
        
        switch (bvh_.GetNodeType(dfi->GetNodeId())) {
            case BVH::NodeType::NT_INTERNAL:
                newNode.primCount = 0;
                newNode.primStartIdx = (unsigned)bvh_.GetNodeSplitAxis(dfi->GetNodeId());
                newNode.next = 0;
                rightChild = bvh_.GetRightChild(dfi->GetNodeId());
                break;
                
            case BVH::NodeType::NT_LEAF:
                bvh_.GetLeafPrimitives(dfi->GetNodeId(), newNode.primStartIdx, newNode.primCount);
                newNode.next = 0xFFFFFFFF;
                rightChild = nullptr;
                break;
        }
        
        nodes_.push_back(newNode);
        rightChildren.push_back(rightChild);
        indices[dfi->GetNodeId()] = nodes_.size() - 1;
        
        
        if (!dfi->HasNext())
            break;
    }

    nodes_[0].next = 0xFFFFFFFF;
    for(int i = 0; i < nodes_.size(); ++i)
    {
        if (nodes_[i].primCount == 0)
        {
            nodes_[i].right = indices[rightChildren[i]];
            nodes_[i + 1].next = indices[rightChildren[i]];
            nodes_[indices[rightChildren[i]]].next = nodes_[i].next;
        }
        else
        {
            nodes_[i].right = 0xFFFFFFFF;
        }
    }
    
    std::cout << "\nBVH has " << nodes_.size() << " nodes\n";
}

OCLBVHBackEnd::Node const*  OCLBVHBackEnd::GetNodes() const
{
    return &nodes_[0];
}

unsigned int OCLBVHBackEnd::GetNodeCount() const
{
    return nodes_.size();
}
