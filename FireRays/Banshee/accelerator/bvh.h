/*
 Banshee and all code, documentation, and other materials contained
 therein are:
 
 Copyright 2015 Dmitry Kozlov
 All Rights Reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are
 met:
 * Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.
 * Neither the name of the software's owners nor the names of its
 contributors may be used to endorse or promote products derived from
 this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 (This is the Modified BSD License)
 */
#ifndef BVH_H
#define BVH_H

#include <memory>
#include <vector>
#include <atomic>


#include "../math/bbox.h"

#ifdef USE_TBB
#define NOMINMAX
#include <tbb/task_group.h>
#include <tbb/mutex.h>
#endif

#include "../primitive/primitive.h"

///< The class represents bounding volume hierarachy
///< intersection accelerator
///<
class Bvh : public Primitive
{
public:
    Bvh(bool usesah = false)
    : root_(nullptr)
    , usesah_(usesah)
    {
    }
    
    ~Bvh();
    // World space bounding box
    bbox const& Bounds() const;
    // Build function: pass bounding boxes and
    void Build(std::vector<Primitive*> const& prims);
    // Intersection test
    bool Intersect(ray& r, float& t, Intersection& isect) const;
    // Intersection check test
    bool Intersect(ray& r) const;
    
protected:
    // Build function
    virtual void BuildImpl(bbox const* bounds, int numbounds);
    // BVH node
    struct Node;
    // Node allocation
    virtual Node* AllocateNode();
    virtual void  InitNodeAllocator(size_t maxnum);
    
    struct SplitRequest
    {
        // Starting index of a request
        int startidx;
        // Number of primitives
        int numprims;
        // Root node
        Node** ptr;
        // Bounding box
        bbox bounds;
        // Centroid bounds
        bbox centroid_bounds;
        // Level
        int level;
    };
    
    struct SahSplit
    {
        int dim;
        float split;
    };
    
    void BuildNode(SplitRequest const& req, bbox const* bounds, float3 const* centroids, int* primindices);
    
    SahSplit FindSahSplit(SplitRequest const& req, bbox const* bounds, float3 const* centroids, int* primindices) const;
    
    // Enum for node type
    enum NodeType
    {
        kInternal,
        kLeaf
    };
    
    // Bvh nodes
    std::vector<Node> nodes_;
    // Identifiers of leaf primitives
    std::vector<int> primids_;
    // Here all the primitives including refined ones
    std::vector<Primitive*> prims_;
    // Here are all the bounds
    std::vector<bbox> bounds_;
    // Here are the primitives to keep track of
    std::vector<std::unique_ptr<Primitive> > primstorage_;
    
#ifdef USE_TBB
    tbb::atomic<int> nodecnt_;
    tbb::mutex primitive_mutex_;
    tbb::task_group taskgroup_;
#else
    // Node allocator counter, atomic for thread safety
    std::atomic<int> nodecnt_;
#endif
    // Bounding box containing all primitives
    bbox bound_;
    // Root node
    Node* root_;
    // SAH flag
    bool usesah_;
    
    
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

inline Bvh::~Bvh()
{
}

#endif // BVH_H