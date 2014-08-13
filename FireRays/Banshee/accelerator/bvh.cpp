#include "bvh.h"

#include <algorithm>
#include <thread>
#include <stack>
#include <numeric>
#include <cassert>

void Bvh::Build(std::vector<Primitive*> const& prims)
{
    // Store all the primitives in storage array
    primitive_storage_.resize(prims.size());
    
    // Reserve
    std::vector<Primitive*> tempprims;
    tempprims.reserve(prims.size());
    primitives_.reserve(prims.size());
    for (int i=0; i<prims.size(); ++i)
    {
        primitive_storage_[i] = std::unique_ptr<Primitive>(prims[i]);
        
        // Refine the primitves
        // TODO: only a single level of indirection for now
        if (prims[i]->intersectable())
        {
            tempprims.push_back(prims[i]);
        }
        else
        {
            prims[i]->Refine(tempprims);
        }
        
        // Calc bbox
        bounds_ = bboxunion(bounds_, prims[i]->Bounds());
    }
    
    BuildImpl(tempprims);
}

bool Bvh::Intersect(ray& r, float& t, Intersection& isect) const
{
    // Check if we have been initialized
    assert(root_);
    // Maintain a stack of nodes to process
    std::stack<Node*> testnodes;
    // Push root
    testnodes.push(root_);
    // Precalc inv ray dir for bbox testing
    float3 invrd = float3(1.f / r.d.x, 1.f / r.d.y, 1.f / r.d.z);
    
    // Hit flag
    bool hit = false;
    // Hit parametric distance
    float tt = r.t.y;
    // Start processing nodes
    while (!testnodes.empty())
    {
        Node* node = testnodes.top();
        testnodes.pop();
        
        if (node->type == kLeaf)
        {
            for (int i = node->startidx; i < node->startidx + node->numprims; ++i)
            {
                if (primitives_[i]->Intersect(r, tt, isect))
                {
                    hit = true;
                    r.t.y = tt;
                }
            }
        }
        else
        {
            if (intersects(r, invrd, node->lc->bounds)) testnodes.push(node->lc);
            if (intersects(r, invrd, node->rc->bounds)) testnodes.push(node->rc);
        }
    }
    
    return hit;
}

bool Bvh::Intersect(ray& r) const
{
    // Check if we have been initialized
    assert(root_);
    // Maintain a stack of nodes to process
    std::stack<Node*> testnodes;
    // Push root
    testnodes.push(root_);
    // Precalc inv ray dir for bbox testing
    float3 invrd = float3(1.f / r.d.x, 1.f / r.d.y, 1.f / r.d.z);
    
    while (!testnodes.empty())
    {
        Node* node = testnodes.top();
        testnodes.pop();
        
        if (node->type == kLeaf)
        {
            for (int i = node->startidx; i < node->startidx + node->numprims; ++i)
            {
                if (primitives_[i]->Intersect(r))
                {
                    return true;
                }
            }
        }
        else
        {
            if (intersects(r, invrd, node->lc->bounds))
                testnodes.push(node->lc);
            if (intersects(r, invrd, node->rc->bounds))
                testnodes.push(node->rc);
        }
    }
    
    return false;
}

bbox Bvh::Bounds() const
{
    return bounds_;
}

void Bvh::BuildImpl(std::vector<Primitive*> const& prims)
{
    // Structure describing split request
    struct SplitRequest
    {
        int startidx;
        int numprims;
        Node** ptr;
    };
    
    // We use indices as primitive references
    std::vector<int> primindices(prims.size());
    std::iota(primindices.begin(), primindices.end(), 0);
    
    SplitRequest init = {0, static_cast<int>(prims.size()), nullptr};
    
    std::stack<SplitRequest> stack;
    // Put initial request into the stack
    stack.push(init);
    
    while (!stack.empty())
    {
        // Fetch new request
        SplitRequest req = stack.top();
        stack.pop();
        
        // Prepare new node
        nodes_.push_back(std::unique_ptr<Node>(new Node));
        Node* node = nodes_.back().get();
        node->bounds = bbox();
        
        // Calc bbox
        // TODO: apply OpenMP reduction
        for (int i = req.startidx; i < req.startidx + req.numprims; ++i)
        {
            node->bounds = bboxunion(node->bounds, prims[primindices[i]]->Bounds());
        }
        
        // Create leaf node if we have enough prims
        if (req.numprims < 2)
        {
            node->type = kLeaf;
            node->startidx = (int)primitives_.size();
            node->numprims = req.numprims;
            for (int i = req.startidx; i < req.startidx + req.numprims; ++i)
            {
                primitives_.push_back(prims[primindices[i]]);
            }
        }
        else
        {
            node->type = kInternal;
            // Create two leafs
            // Choose the maximum extend
            int axis = node->bounds.maxdim();
            float border = node->bounds.center()[axis];
            auto part = std::partition(primindices.begin() + req.startidx, primindices.begin() + req.startidx + req.numprims, [&,axis,border](int i)
                           {
                               bbox b = prims[primindices[i]]->Bounds();
                               return b.center()[axis] < border;
                           }
                           );
            
            // Find split index relative to req.startidx
            int idx = (int)(part - (primindices.begin() + req.startidx));
            
            // If we have not split anything use split in halves
            if (idx == 0
                || idx == req.numprims)
            {
                idx = req.numprims >> 1;
            }
            
            // Left request
            SplitRequest leftrequest = {req.startidx, idx, &node->lc};
            // Right request
            SplitRequest rightrequest = {req.startidx + idx, req.numprims - idx,  &node->rc};
            
            // Put those to stack
            stack.push(leftrequest);
            stack.push(rightrequest);
        }
        
        // Set parent ptr if any
        if (req.ptr) *req.ptr = node;
    }
    
    // Set root_ pointer
    root_ = nodes_[0].get();
}





