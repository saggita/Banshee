#include "BVH.h"

#include <algorithm>

struct BVH::Node
{
    BBox        box;
    NodeType   type;
    
    union
    {
        struct
        {
            SplitAxis splitAxis;
            Node* child[2];
        };
        
        struct
        {
            unsigned primStartIdx;
            unsigned primCount;
        };
    };
    
    Node(NodeType t, BBox const& b)
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

BVH::BVH()
: root_(nullptr)
{
}

BVH::~BVH()
{
}

BVH::NodeId BVH::CreateInternalNode(NodeId id, ChildRel rel, SplitAxis axis, BBox const& b)
{
    if (id == GetPreRootNode())
    {
        assert(!root_);
        root_ = new Node(NodeType::NT_INTERNAL, b);
        root_->splitAxis = axis;
        nodeCache_.emplace(root_);
        
        // Return node's id
        return root_;
    }
    else
    {
    
    // Check if this is really a node
    if (nodeCache_.find(id) != nodeCache_.end())
    {
        Node* node = static_cast<Node*>(id);
        
        // We can attach internal node to internal nodes only
        assert(node->type == NodeType::NT_INTERNAL);
        // And if there's no other node attached on a given side
        assert(!node->child[rel]);
        
        // Attach a new internal node
        node->child[rel] = new Node(NodeType::NT_INTERNAL, b);
        node->child[rel]->splitAxis = axis;
        
        // Chech invariant
        //assert(CheckInvariant(node));
        
        // Cache node
        nodeCache_.emplace(node->child[rel]);
        
        return node->child[rel];
    }
    else
        throw std::runtime_error("No such node");
    }
}

BVH::NodeId BVH::CreateLeafNode(NodeId id, ChildRel rel, BBox const& b, unsigned* primIndices, unsigned idxCount)
{
    // Is the node valid ptr?
    assert(id);
    assert(idxCount > 0);
    assert(primIndices);
    
    // Check if this is really a node
    if (nodeCache_.find(id) != nodeCache_.end())
    {
        Node* node = static_cast<Node*>(id);
        
        // We can attach leaf to internal nodes only
        assert(node->type == NodeType::NT_INTERNAL);
        // And if there's no other node attached on a given side
        assert(!node->child[rel]);
        
        // Attach a new internal node
        node->child[rel] = new Node(NodeType::NT_LEAF, b);
        
        // Copy indices into the internal array
        unsigned primStartIdx = static_cast<unsigned>(primIndices_.size());
        for (unsigned i = 0; i < idxCount; ++i)
        {
            primIndices_.push_back(primIndices[i]);
        }
        
        // Fill in primitive data
        node->child[rel]->primStartIdx = primStartIdx;
        node->child[rel]->primCount = idxCount;
        
        // Cache node
        nodeCache_.emplace(node->child[rel]);
        
        return node->child[rel];
    }
    else
        throw std::runtime_error("No such node");
    
}

bool   BVH::CheckInvariant(NodeId id)
{
    // Is the node valid ptr?
    assert(id);
    Node* node = static_cast<Node*>(id);
    
    assert(node->type == NodeType::NT_INTERNAL);
    
    bool leftInvariant = true;
    bool rightInvariant = true;
    
    if (node->child[CR_LEFT])
    {
        leftInvariant = leftInvariant && Contains(node->box, node->child[CR_LEFT]->box);
    }
    
    if (node->child[CR_RIGHT])
    {
        rightInvariant = rightInvariant && Contains(node->box, node->child[CR_RIGHT]->box);
    }

    return leftInvariant && rightInvariant;
}

BBox const& BVH::GetNodeBbox(NodeId id) const
{
    assert(id);
    Node* node = static_cast<Node*>(id);
    return node->box;
}

BVH::NodeType    BVH::GetNodeType(NodeId id) const
{
    assert(id);
    Node* node = static_cast<Node*>(id);
    return node->type;
}

BVH::NodeId      BVH::GetRightChild(NodeId id) const
{
    assert(id);
    Node* node = static_cast<Node*>(id);
    
    assert(node->type == NodeType::NT_INTERNAL);
    return node->child[CR_RIGHT];
}

BVH::NodeId      BVH::GetLeftChild(NodeId id) const
{
    assert(id);
    Node* node = static_cast<Node*>(id);
    
    assert(node->type == NodeType::NT_INTERNAL);
    return node->child[CR_LEFT];
}

BVH::SplitAxis   BVH::GetNodeSplitAxis(NodeId id) const
{
    assert(id);
    Node* node = static_cast<Node*>(id);
    
    assert(node->type == NodeType::NT_INTERNAL);
    return node->splitAxis;
}

unsigned    BVH::GetLeafPrimitiveCount(NodeId id) const
{
    assert(id);
    Node* node = static_cast<Node*>(id);
    
    assert(node->type == NodeType::NT_LEAF);
    return node->primCount;
}

void        BVH::GetLeafPrimitives(NodeId id, std::vector<unsigned>& v) const
{
    assert(id);
    Node* node = static_cast<Node*>(id);
    
    assert(node->type == NodeType::NT_LEAF);
    
    for (unsigned i = 0; i < node->primCount; ++i)
    {
        v.push_back(primIndices_[node->primStartIdx + i]);
    }
}

void        BVH::GetLeafPrimitives(NodeId id, unsigned* prims) const
{
    assert(id);
    Node* node = static_cast<Node*>(id);
    
    assert(node->type == NodeType::NT_LEAF);
    
    for (unsigned i = 0; i < node->primCount; ++i)
    {
        prims[i] = primIndices_[node->primStartIdx + i];
    }
}

void   BVH::GetLeafPrimitives(NodeId id, unsigned& primStartIdx, unsigned& primCount) const
{
    assert(id);
    Node* node = static_cast<Node*>(id);
    
    assert(node->type == NodeType::NT_LEAF);
    
    primStartIdx = node->primStartIdx;
    primCount = node->primCount;
}

unsigned const*   BVH::GetPrimitiveIndices() const
{
    return &primIndices_[0];
}

unsigned          BVH::GetPrimitiveIndexCount() const
{
    return primIndices_.size();
}


class DepthFirstIterator: public BVH::Iterator
{
public:
    DepthFirstIterator (BVH::Node* root)
    : root_(root)
    {
        // this is not really a virtual call
        Reset();
    }
    
    BVH::NodeId GetNodeId()
    {
        return current_;
    }

    void   Reset()
    {
        // Clear the stack: call optimized move constructor
        nodesToProcess_ = std::stack<BVH::Node*>();
        current_ = root_;
        EmplaceChildren(current_);
    }
    
    bool   HasNext()
    {
        return nodesToProcess_.size() > 0;
    }
    
    void   Next()
    {
        current_ = nodesToProcess_.top();
        nodesToProcess_.pop();
        EmplaceChildren(current_);
    }
    
    void EmplaceChildren(BVH::Node* node)
    {
        if (node->type == BVH::NodeType::NT_INTERNAL)
        {
            nodesToProcess_.push(node->child[BVH::CR_RIGHT]);
            nodesToProcess_.push(node->child[BVH::CR_LEFT]);
        }
    }
    
    BVH::Node* root_;
    BVH::Node* current_;
    std::stack<BVH::Node*> nodesToProcess_;
};

class BreadthFirstIterator: public BVH::Iterator
{
public:
    BreadthFirstIterator (BVH::Node* root)
    : root_(root)
    {
        // this is not really a virtual call
        Reset();
    }
    
    BVH::NodeId GetNodeId()
    {
        return current_;
    }
    
    void   Reset()
    {
        // Clear the stack: call optimized move constructor
        nodesToProcess_ = std::queue<BVH::Node*>();
        current_ = root_;
        EmplaceChildren(current_);
    }
    
    bool   HasNext()
    {
        return nodesToProcess_.size() > 0;
    }
    
    
    
    void   Next()
    {
        current_ = nodesToProcess_.front();
        nodesToProcess_.pop();
        EmplaceChildren(current_);
    }
    
    void EmplaceChildren(BVH::Node* node)
    {
        if (node->type == BVH::NodeType::NT_INTERNAL)
        {
            nodesToProcess_.push(node->child[BVH::CR_LEFT]);
            nodesToProcess_.push(node->child[BVH::CR_RIGHT]);
        }
    }
    
    BVH::Node* root_;
    BVH::Node* current_;
    std::queue<BVH::Node*> nodesToProcess_;
};


BVH::Iterator*   BVH::CreateBreadthFirstIterator()
{
    return new BreadthFirstIterator(root_);
}

BVH::Iterator*   BVH::CreateDepthFirstIterator()
{
    return new DepthFirstIterator(root_);
}

void BVH::DestroyIterator(Iterator* iter)
{
    delete iter;
}



// Intersect ray with the axis-aligned box
static bool Intersects(BVH::RayQuery& q, BBox const& sBox)
{
    vector3 vRayDir = vector3(1.f / q.d.x(), 1.f / q.d.y(), 1.f / q.d.z());
    float lo = vRayDir.x()*(sBox.GetMinPoint().x() - q.o.x());
    float hi = vRayDir.x()*(sBox.GetMaxPoint().x() - q.o.x());

    float tmin = std::min(lo, hi);
    float tmax = std::max(lo, hi);

    float lo1 = vRayDir.y()*(sBox.GetMinPoint().y() - q.o.y());
    float hi1 = vRayDir.y()*(sBox.GetMaxPoint().y() - q.o.y());

    tmin = std::max(tmin, std::min(lo1, hi1));
    tmax = std::min(tmax, std::max(lo1, hi1));

    float lo2 = vRayDir.z()*(sBox.GetMinPoint().z() - q.o.z());
    float hi2 = vRayDir.z()*(sBox.GetMaxPoint().z() - q.o.z());

    tmin = std::max(tmin, std::min(lo2, hi2));
    tmax = std::min(tmax, std::max(lo2, hi2));

    if ((tmin <= tmax) && (tmax > 0.f))
    {
        return (tmin >= 0) ? (tmin < q.t) : (tmax < q.t);
    }
    else
        return false;
}

bool Intersects(BVH::RayQuery& q, vector3 vP1, vector3 vP2, vector3 vP3)
{
    vector3 vE1 = vP2 - vP1;
    vector3 vE2 = vP3 - vP1;
    
    vector3 vS1 = cross(q.d, vE2);
    float  fInvDir = 1.0/dot(vS1, vE1);
    
    vector3 vD = q.o - vP1;
    float  fB1 = dot(vD, vS1) * fInvDir;
    
    if (fB1 < 0.0 || fB1 > 1.0)
        return false;
    
    vector3 vS2 = cross(vD, vE1);
    float  fB2 = dot(q.d, vS2) * fInvDir;
    
    if (fB2 < 0.0 || fB1 + fB2 > 1.0)
        return false;
    
    float fTemp = dot(vE2, vS2) * fInvDir;
    
    if (fTemp > 0 && fTemp <= q.t)
    {
        q.t = fTemp;
        return true;
    }
    
    return false;
}


void        BVH::CastRay(RayQuery& q, RayQueryStatistics& stat, SceneBase::Vertex const* vertices, unsigned const* indices) const
{
    std::stack<BVH::Node*> nodesToProcess_;
    nodesToProcess_.push(root_);

    stat.hitBvh = false;
    stat.hitPrim = false;
    stat.maxDepthVisited = 0;
    stat.numNodesVisited = 0;
    stat.numTrianglesTested = 0;

    while (nodesToProcess_.size() > 0)
    {
        BVH::Node* node = nodesToProcess_.top();
        nodesToProcess_.pop();

        if (Intersects(q, node->box))
        {
            stat.hitBvh = true;
            stat.numNodesVisited++;
            stat.maxDepthVisited = std::max(stat.maxDepthVisited, (unsigned)nodesToProcess_.size());

            if (node->type == BVH::NodeType::NT_LEAF)
            {
                stat.numTrianglesTested += node->primCount;
                for (int i = 0; i < node->primCount; ++i)
                {
                    vector3 v1, v2, v3;
                    unsigned primIdx = primIndices_[node->primStartIdx + i];
                    v1 = vertices[indices[primIdx * 3]].position;
                    v2 = vertices[indices[primIdx * 3 + 1]].position;
                    v3 = vertices[indices[primIdx * 3 + 2]].position;
                    
                    stat.hitPrim = stat.hitPrim || Intersects(q, v1, v2, v3);
                }
            }
            else
            {
                if ( q.d[(int)node->splitAxis] > 0 )
                {
                    nodesToProcess_.push(node->child[BVH::CR_RIGHT]);
                    nodesToProcess_.push(node->child[BVH::CR_LEFT]);
                }
                else
                {
                    nodesToProcess_.push(node->child[BVH::CR_LEFT]);
                    nodesToProcess_.push(node->child[BVH::CR_RIGHT]);
                }
            }
        }
    }
}