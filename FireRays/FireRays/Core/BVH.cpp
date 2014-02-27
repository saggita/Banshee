#include "BVH.h"

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
        assert(CheckInvariant(node));
        
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