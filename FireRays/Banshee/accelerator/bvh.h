#ifndef BVH_H
#define BVH_H

#include "../math/bbox.h"
#include "../primitive/primitive.h"

#include <vector>
#include <set>

class Bvh : public Primitive
{
public:
    Bvh();
    ~Bvh();

    bool Intersect(ray& r, IntersectionApi::Intersection& isect) const;
    bool Intersect(ray& r) const;

protected:
    friend class SplitBVHBuilder;

    enum ChildRel
    {
        CR_LEFT,
        CR_RIGHT
    };

    enum class NodeType
    {
        NT_INTERNAL,
        NT_LEAF
    };

    enum class SplitAxis
    {
        SA_X,
        SA_Y,
        SA_Z
    };

    typedef void* NodeId;
    // Main builder interface
    // Get the id preceeding to root node, this is needed to initialize root
    NodeId GetPreRootNode() const;
    // Create internal node and attach to the node identified by id to the side
    // identified by rel (left or right)
    NodeId CreateInternalNode(NodeId id, ChildRel rel, SplitAxis axis, bbox const& b);
    // Create leaf node and attach along with a list of primitive indices
    NodeId CreateLeafNode(NodeId id, ChildRel rel, bbox const& b, unsigned* primIndices, unsigned idxCount);
    // Check whether node childrens' bboxes are contained within its own bounding box
    bool   CheckInvariant(NodeId id);

private:
    Bvh(Bvh const&);
    Bvh& operator = (Bvh const&);

    // Node descriptor
    struct Node;
    
    // Root node
    Node*                   root_;
    // Node cache to check an existence, should be removed later on
    std::set<NodeId>        node_cache_;
    // Packed primitive indices, leafs reference prim indices in this array
    // by specifying start index and number of primitives in the leaf
    std::vector<unsigned>   prim_indices_;
};


inline Bvh::NodeId Bvh::GetPreRootNode() const
{
    return nullptr;
}

#endif // BVH_H