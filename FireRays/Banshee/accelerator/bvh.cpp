#include "Bvh.h"

#include <algorithm>
#include <cassert>

struct Bvh::Node
{
    bbox        box;
    NodeType    type;
    
    union
    {
        struct
        {
            SplitAxis splitaxis;
            Node*     child[2];
        };
        
        struct
        {
            unsigned startidx;
            unsigned primcount;
        };
    };
    
    Node(NodeType t, bbox const& b)
    : box(b)
    , type(t)
    {
        child[0] = nullptr;
        child[1] = nullptr;
    }
    
    ~Node()
    {
        delete child[0];
        delete child[1];
    }
};

Bvh::Bvh()
: root_(nullptr)
{
}

Bvh::~Bvh()
{
}

Bvh::NodeId Bvh::CreateInternalNode(NodeId id, ChildRel rel, SplitAxis axis, bbox const& b)
{
    if (id == GetPreRootNode())
    {
        assert(!root_);
        root_ = new Node(NodeType::NT_INTERNAL, b);
        root_->splitaxis = axis;
        node_cache_.emplace(root_);
        
        // Return node's id
        return root_;
    }
    else
    {
    
    // Check if this is really a node
    if (node_cache_.find(id) != node_cache_.end())
    {
        Node* node = static_cast<Node*>(id);
        
        // We can attach internal node to internal nodes only
        assert(node->type == NodeType::NT_INTERNAL);
        // And if there's no other node attached on a given side
        assert(!node->child[rel]);
        
        // Attach a new internal node
        node->child[rel] = new Node(NodeType::NT_INTERNAL, b);
        node->child[rel]->splitaxis = axis;
        
        // Chech invariant
        //assert(CheckInvariant(node));
        
        // Cache node
        node_cache_.emplace(node->child[rel]);
        
        return node->child[rel];
    }
    else
        throw std::runtime_error("No such node");
    }
}

Bvh::NodeId Bvh::CreateLeafNode(NodeId id, ChildRel rel, bbox const& b, unsigned* indices, unsigned idxcount)
{
    // Is the node valid ptr?
    assert(id);
    assert(idxcount > 0);
    assert(indices);
    
    // Check if this is really a node
    if (node_cache_.find(id) != node_cache_.end())
    {
        Node* node = static_cast<Node*>(id);
        
        // We can attach leaf to internal nodes only
        assert(node->type == NodeType::NT_INTERNAL);
        // And if there's no other node attached on a given side
        assert(!node->child[rel]);
        
        // Attach a new internal node
        node->child[rel] = new Node(NodeType::NT_LEAF, b);
        
        // Copy indices into the internal array
        unsigned primStartIdx = static_cast<unsigned>(prim_indices_.size());
        for (unsigned i = 0; i < idxcount; ++i)
        {
            prim_indices_.push_back(indices[i]);
        }
        
        // Fill in primitive data
        node->child[rel]->startidx = primStartIdx;
        node->child[rel]->primcount = idxcount;
        
        // Cache node
        node_cache_.emplace(node->child[rel]);
        
        return node->child[rel];
    }
    else
        throw std::runtime_error("No such node");
    
}

bool   Bvh::CheckInvariant(NodeId id)
{
    // Is the node valid ptr?
    assert(id);
    Node* node = static_cast<Node*>(id);
    
    assert(node->type == NodeType::NT_INTERNAL);
    
    bool li = true;
    bool ri = true;
    
    if (node->child[CR_LEFT])
    {
        li = li && contains(node->box, node->child[CR_LEFT]->box);
    }
    
    if (node->child[CR_RIGHT])
    {
        ri = ri && contains(node->box, node->child[CR_RIGHT]->box);
    }

    return li && ri;
}




//bool Intersects(Bvh::RayQuery& q, vector3 vPP1, vector3 vPP2, vector3 vPP3)
//{
//    vector3opt vP1 = vector3opt(vPP1.x(), vPP1.y(), vPP1.z());
//    vector3opt vP2= vector3opt(vPP2.x(), vPP2.y(), vPP2.z());
//    vector3opt vP3 = vector3opt(vPP3.x(), vPP3.y(), vPP3.z());
//    vector3opt vE1 = vP2 - vP1;
//    vector3opt vE2 = vP3 - vP1;
//
//    vector3opt vS1 = cross(q.d, vE2);
//    float  fInvDir = 1.0/dot(vS1, vE1);
//    
//    vector3opt vD = q.o - vP1;
//    float  fB1 = dot(vD, vS1) * fInvDir;
//    
//    if (fB1 < 0.0 || fB1 > 1.0)
//        return false;
//    
//    vector3opt vS2 = cross(vD, vE1);
//    float  fB2 = dot(q.d, vS2) * fInvDir;
//    
//    if (fB2 < 0.0 || fB1 + fB2 > 1.0)
//        return false;
//    
//    float fTemp = dot(vE2, vS2) * fInvDir;
//    
//    if (fTemp > 0 && fTemp <= q.t)
//    {
//        q.t = fTemp;
//        return true;
//    }
//    
//    return false;
//}

//
//void        Bvh::CastRay(RayQuery& q, RayQueryStatistics& stat, SceneBase::Vertex const* vertices, unsigned const* indices) const
//{
//    std::stack<Bvh::Node*> nodesToProcess_;
//    nodesToProcess_.push(root_);
//
//    stat.hitBvh = false;
//    stat.hitPrim = false;
//    stat.maxDepthVisited = 0;
//    stat.numNodesVisited = 0;
//    stat.numTrianglesTested = 0;
//
//    while (nodesToProcess_.size() > 0)
//    {
//        Bvh::Node* node = nodesToProcess_.top();
//        nodesToProcess_.pop();
//
//        if (Intersects(q, node->box))
//        {
//            stat.hitBvh = true;
//            stat.numNodesVisited++;
//            stat.maxDepthVisited = std::max(stat.maxDepthVisited, (unsigned)nodesToProcess_.size());
//
//            if (node->type == Bvh::NodeType::NT_LEAF)
//            {
//                stat.numTrianglesTested += node->primCount;
//                for (int i = 0; i < node->primCount; ++i)
//                {
//                    vector3 v1, v2, v3;
//                    unsigned primIdx = primIndices_[node->primStartIdx + i];
//                    v1 = vertices[indices[primIdx * 3]].position;
//                    v2 = vertices[indices[primIdx * 3 + 1]].position;
//                    v3 = vertices[indices[primIdx * 3 + 2]].position;
//                    
//                    stat.hitPrim = stat.hitPrim || Intersects(q, v1, v2, v3);
//                }
//            }
//            else
//            {
//                if ( q.d[(int)node->splitAxis] > 0 )
//                {
//                    nodesToProcess_.push(node->child[Bvh::CR_RIGHT]);
//                    nodesToProcess_.push(node->child[Bvh::CR_LEFT]);
//                }
//                else
//                {
//                    nodesToProcess_.push(node->child[Bvh::CR_LEFT]);
//                    nodesToProcess_.push(node->child[Bvh::CR_RIGHT]);
//                }
//            }
//        }
//    }
//}

bool Bvh::Intersect(ray& r, IntersectionApi::Intersection& isect) const
{
    return false;
}

bool Bvh::Intersect(ray& ray) const
{
    return false;
}