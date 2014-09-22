#ifndef BVH_H
#define BVH_H

#include <memory>
#include <vector>

#include "../math/bbox.h"
#include "../primitive/primitive.h"

///< The class represents bounding volume hierarachy
///< intersection accelerator
///<
class Bvh : public Primitive
{
public:
    Bvh()
    : root_(nullptr)
    {
    }
    // Intersection test
    bool Intersect(ray& r, float& t, Intersection& isect) const;
    // Intersection check test
    bool Intersect(ray& r) const;
    // World space bounding box
    bbox Bounds() const;
    // Build function
    void Build(std::vector<Primitive*> const& prims);
    
    // Query BVH statistics
    struct Statistics;
    void QueryStatistics(Statistics& stat) const;
    
    
protected:
    // Build function
    virtual void BuildImpl(std::vector<Primitive*> const& prims);
    // BVH node
    struct Node;
    // Enum for node type
    enum NodeType
    {
        kInternal,
        kLeaf
    };
    
    // Primitves owned by this instance
    std::vector<std::unique_ptr<Primitive> > primitive_storage_;
    // Primitves to intersect (includes refined prims)
    std::vector<Primitive*> primitives_;
    // BVH nodes
    std::vector<std::unique_ptr<Node> > nodes_;
    // Bounding box
    bbox bounds_;
    // Root node
    Node* root_;
    
private:
    Bvh(Bvh const&);
    Bvh& operator = (Bvh const&);
};

struct Bvh::Node
{
    // Node bounds in world space
    bbox bounds;
    // Type of the node
    NodeType type;
    union
    {
        // For internal nodes: left and right children
        struct
        {
            Node* lc;
            Node* rc;
        };
        
        // For leaves: starting primitive index and number of primitives
        struct
        {
            int startidx;
            int numprims;
        };
    };
};

struct Bvh::Statistics
{
    int internalcount;
    int leafcount;
    float minoverlaparea;
    float maxoverlaparea;
    float avgoverlaparea;
};

#endif // BVH_H