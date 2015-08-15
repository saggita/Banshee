#include "bvh.h"

#include <algorithm>
#include <thread>
#include <stack>
#include <numeric>
#include <cassert>
#include <vector>
#include <future>

static bool is_nan(float v)
{
    return v != v;
}

void Bvh::Build(std::vector<Primitive*> const& prims)
{
    // Store all the primitives in storage array
    primstorage_.resize(prims.size());
    
    // Reserve some space
    prims_.reserve(prims.size());
    
    // Now refine primitives
    for (int i=0; i<prims.size(); ++i)
    {
        // For root prim we need to manage memory
        primstorage_[i] = std::unique_ptr<Primitive>(prims[i]);
        
        // Refine the primitve if it is not intersectable
        if (prims[i]->intersectable())
        {
            prims_.push_back(prims[i]);
        }
        else
        {
            prims[i]->Refine(prims_);
        }
    }
    
    // Reserve space for bounds
    bounds_.resize(prims_.size());
    
    // Calculate bounds
    for (int i=0; i<prims_.size(); ++i)
    {
        bounds_[i] = prims_[i]->Bounds();
        bound_.grow(bounds_[i]);
    }
    
    // Enlarge bounding box a bit
    bound_.grow(bound_.pmin + 1.05f * bound_.extents());
    bound_.grow(bound_.pmin - 0.05f * bound_.extents());
    
    BuildImpl(&bounds_[0], (int)bounds_.size());
}

bbox const& Bvh::Bounds() const
{
    return bound_;
}


void  Bvh::InitNodeAllocator(size_t maxnum)
{
    nodecnt_ = 0;
    nodes_.resize(maxnum);
}

Bvh::Node* Bvh::AllocateNode()
{
    return &nodes_[nodecnt_++];
}

void Bvh::BuildNode(SplitRequest const& req, bbox const* bounds, float3 const* centroids, int* primindices)
{
    Node* node = AllocateNode();
    node->bounds = req.bounds;
    
    // Create leaf node if we have enough prims
    if (req.numprims < 2)
    {
#ifdef USE_TBB
        primitive_mutex_.lock();
#endif
        node->type = kLeaf;
        node->startidx = req.startidx;
        node->numprims = req.numprims;
#ifdef USE_TBB
        primitive_mutex_.unlock();
#endif
    }
    else
    {
        node->type = kInternal;
        
        // Choose the maximum extent
        int axis = req.centroid_bounds.maxdim();
        float border = req.centroid_bounds.center()[axis];
        
        if (usesah_ && req.level < 10)
        {
            SahSplit ss = FindSahSplit(req, bounds, centroids, primindices);
            
            if (!is_nan(ss.split))
            {
                axis = ss.dim;
                border = ss.split;
            }
        }
        
        // Start partitioning and updating extents for children at the same time
        bbox leftbounds, rightbounds, leftcentroid_bounds, rightcentroid_bounds;
        int splitidx = req.startidx;
        
        bool near2far = (req.numprims + req.startidx) & 0x1;
        
        if (req.centroid_bounds.extents()[axis] > 0.f)
        {
            auto first = req.startidx;
            auto last = req.startidx + req.numprims;
            
            if (near2far)
            {
                while (1)
                {
                    while ((first != last) &&
                           centroids[primindices[first]][axis] < border)
                    {
                        leftbounds.grow(bounds[primindices[first]]);
                        leftcentroid_bounds.grow(centroids[primindices[first]]);
                        ++first;
                    }
                    
                    if (first == last--) break;
                    
                    rightbounds.grow(bounds[primindices[first]]);
                    rightcentroid_bounds.grow(centroids[primindices[first]]);
                    
                    while ((first != last) &&
                           centroids[primindices[last]][axis] >= border)
                    {
                        rightbounds.grow(bounds[primindices[last]]);
                        rightcentroid_bounds.grow(centroids[primindices[last]]);
                        --last;
                    }
                    
                    if (first == last) break;
                    
                    leftbounds.grow(bounds[primindices[last]]);
                    leftcentroid_bounds.grow(centroids[primindices[last]]);
                    
                    std::swap(primindices[first++], primindices[last]);
                }
            }
            else
            {
                while (1)
                {
                    while ((first != last) &&
                           centroids[primindices[first]][axis] >= border)
                    {
                        leftbounds.grow(bounds[primindices[first]]);
                        leftcentroid_bounds.grow(centroids[primindices[first]]);
                        ++first;
                    }
                    
                    if (first == last--) break;
                    
                    rightbounds.grow(bounds[primindices[first]]);
                    rightcentroid_bounds.grow(centroids[primindices[first]]);
                    
                    while ((first != last) &&
                           centroids[primindices[last]][axis] < border)
                    {
                        rightbounds.grow(bounds[primindices[last]]);
                        rightcentroid_bounds.grow(centroids[primindices[last]]);
                        --last;
                    }
                    
                    if (first == last) break;
                    
                    leftbounds.grow(bounds[primindices[last]]);
                    leftcentroid_bounds.grow(centroids[primindices[last]]);
                    
                    std::swap(primindices[first++], primindices[last]);
                }
            }
            
            splitidx = first;
        }
        
        if (splitidx == req.startidx || splitidx == req.startidx + req.numprims)
        {
            splitidx = req.startidx + (req.numprims >> 1);
            
            for (int i = req.startidx; i < splitidx; ++i)
            {
                leftbounds.grow(bounds[primindices[i]]);
                leftcentroid_bounds.grow(centroids[primindices[i]]);
            }
            
            for (int i = splitidx; i < req.startidx + req.numprims; ++i)
            {
                rightbounds.grow(bounds[primindices[i]]);
                rightcentroid_bounds.grow(centroids[primindices[i]]);
            }
        }
        
        // Left request
        SplitRequest leftrequest = { req.startidx, splitidx - req.startidx, &node->lc, leftbounds, leftcentroid_bounds, req.level + 1 };
        // Right request
        SplitRequest rightrequest = { splitidx, req.numprims - (splitidx - req.startidx), &node->rc, rightbounds, rightcentroid_bounds, req.level + 1 };
        
#ifdef USE_TBB
        // Put those to stack
        // std::vector<std::future<int> > futures;
        if (leftrequest.numprims > 4096 * 4)
        {
            taskgroup_.run(
                           [=](){
                               //std::cout << "Handling left " << leftrequest.startidx << " " << leftrequest.numprims << std::endl;
                               BuildNode(leftrequest, bounds, centroids, primindices);
                           });
        }
        else
#endif
        {
            // Put those to stack
            BuildNode(leftrequest, bounds, centroids, primindices);
        }
        
#ifdef USE_TBB
        if (rightrequest.numprims > 4096 * 4 )
        {
            
            taskgroup_.run(
                           [=](){
                               //std::cout << "Handling right " << rightrequest.startidx << " " << rightrequest.numprims << std::endl;
                               BuildNode(rightrequest, bounds, centroids, primindices);
                           });
        }
        else
#endif
        {
            BuildNode(rightrequest, bounds, centroids, primindices);
        }
    }
    
    // Set parent ptr if any
    if (req.ptr) *req.ptr = node;
}

Bvh::SahSplit Bvh::FindSahSplit(SplitRequest const& req, bbox const* bounds, float3 const* centroids, int* primindices) const
{
    // SAH implementation
    // calc centroids histogram
    int const kNumBins = 64;
    // moving split bin index
    int splitidx = -1;
    // Set SAH to maximum float value as a start
    float sah = std::numeric_limits<float>::max();
    SahSplit split;
    split.dim = 0;
    split.split = std::numeric_limits<float>::quiet_NaN();
    
    // if we cannot apply histogram algorithm
    // put NAN sentinel as split border
    // PerformObjectSplit simply splits in half
    // in this case
    float3 centroid_extents = req.centroid_bounds.extents();
    if (centroid_extents.sqnorm() == 0.f)
    {
        return split;
    }
    
    // Bin has bbox and occurence count
    struct Bin
    {
        bbox bounds;
        int count;
    };
    
    // Keep bins for each dimension
    Bin   bins[3][kNumBins];
    // Precompute inverse parent area
    float invarea = 1.f / req.bounds.surface_area();
    // Precompute min point
    float3 rootmin = req.centroid_bounds.pmin;
    
    // Evaluate all dimensions
    for (int axis = 0; axis < 3; ++axis)
    {
        float rootminc = rootmin[axis];
        // Range for histogram
        float centroid_rng = centroid_extents[axis];
        float invcentroid_rng = 1.f / centroid_rng;
        
        // If the box is degenerate in that dimension skip it
        if (centroid_rng == 0.f) continue;
        
        // Initialize bins
        for (unsigned i = 0; i < kNumBins; ++i)
        {
            bins[axis][i].count = 0;
            bins[axis][i].bounds = bbox();
        }
        
        // Calc primitive refs histogram
        for (int i = req.startidx; i < req.startidx + req.numprims; ++i)
        {
            int idx = primindices[i];
            int binidx = (int)std::min<float>(kNumBins * ((centroids[idx][axis] - rootminc) * invcentroid_rng), kNumBins - 1);
            
            ++bins[axis][binidx].count;
            bins[axis][binidx].bounds.grow(bounds[idx]);
        }
        
        bbox rightbounds[kNumBins - 1];
        
        // Start with 1-bin right box
        bbox rightbox = bbox();
        for (int i = kNumBins - 1; i > 0; --i)
        {
            rightbox.grow(bins[axis][i].bounds);
            rightbounds[i - 1] = rightbox;
        }
        
        bbox leftbox = bbox();
        int  leftcount = 0;
        int  rightcount = req.numprims;
        
        // Start best SAH search
        // i is current split candidate (split between i and i + 1)
        float sahtmp = 0.f;
        for (int i = 0; i < kNumBins - 1; ++i)
        {
            leftbox.grow(bins[axis][i].bounds);
            leftcount += bins[axis][i].count;
            rightcount -= bins[axis][i].count;
            
            // Compute SAH
            sahtmp = 10.f + (leftcount * leftbox.surface_area() + rightcount * rightbounds[i].surface_area()) * invarea;
            
            // Check if it is better than what we found so far
            if (sahtmp < sah)
            {
                split.dim = axis;
                splitidx = i;
                sah = sahtmp;
            }
        }
    }
    
    // Choose split plane
    if (splitidx != -1)
    {
        split.split = rootmin[split.dim] + (splitidx + 1) * (centroid_extents[split.dim] / kNumBins);
    }
    
    return split;
}

void Bvh::BuildImpl(bbox const* bounds, int numbounds)
{
    // Structure describing split request
    InitNodeAllocator(2 * numbounds - 1);
    
    // Cache some stuff to have faster partitioning
    std::vector<float3> centroids(numbounds);
    primids_.resize(numbounds);
    std::iota(primids_.begin(), primids_.end(), 0);
    
    // Calc bbox
    bbox centroid_bounds;
    for (size_t i = 0; i < numbounds; ++i)
    {
        float3 c = bounds[i].center();
        centroid_bounds.grow(c);
        centroids[i] = c;
    }
    
    SplitRequest init = { 0, numbounds, nullptr, bound_, centroid_bounds, 0 };
    
#ifdef USE_BUILD_STACK
    std::stack<SplitRequest> stack;
    // Put initial request into the stack
    stack.push(init);
    
    while (!stack.empty())
    {
        // Fetch new request
        SplitRequest req = stack.top();
        stack.pop();
        
        Node* node = AllocateNode();
        node->bounds = req.bounds;
        
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
            
            // Choose the maximum extent
            int axis = req.centroid_bounds.maxdim();
            float border = req.centroid_bounds.center()[axis];
            
            // Start partitioning and updating extents for children at the same time
            bbox leftbounds, rightbounds, leftcentroid_bounds, rightcentroid_bounds;
            int splitidx = 0;
            
            if (req.centroid_bounds.extents()[axis] > 0.f)
            {
                
                auto first = req.startidx;
                auto last = req.startidx + req.numprims;
                
                while (1)
                {
                    while ((first != last) &&
                           centroids[primindices[first]][axis] < border)
                    {
                        leftbounds.grow(bounds[primindices[first]]);
                        leftcentroid_bounds.grow(centroids[primindices[first]]);
                        ++first;
                    }
                    
                    if (first == last--) break;
                    
                    rightbounds.grow(bounds[primindices[first]]);
                    rightcentroid_bounds.grow(centroids[primindices[first]]);
                    
                    while ((first != last) &&
                           centroids[primindices[last]][axis] >= border)
                    {
                        rightbounds.grow(bounds[primindices[last]]);
                        rightcentroid_bounds.grow(centroids[primindices[last]]);
                        --last;
                    }
                    
                    if (first == last) break;
                    
                    leftbounds.grow(bounds[primindices[last]]);
                    leftcentroid_bounds.grow(centroids[primindices[last]]);
                    
                    std::swap(primindices[first++], primindices[last]);
                }
                
                splitidx = first;
            }
            else
            {
                splitidx = req.startidx + (req.numprims >> 1);
                
                for (int i = req.startidx; i < splitidx; ++i)
                {
                    leftbounds.grow(bounds[primindices[i]]);
                    leftcentroid_bounds.grow(centroids[primindices[i]]);
                }
                
                for (int i = splitidx; i < req.startidx + req.numprims; ++i)
                {
                    rightbounds.grow(bounds[primindices[i]]);
                    rightcentroid_bounds.grow(centroids[primindices[i]]);
                }
            }
            
            // Left request
            SplitRequest leftrequest = { req.startidx, splitidx - req.startidx, &node->lc, leftbounds, leftcentroid_bounds };
            // Right request
            SplitRequest rightrequest = { splitidx, req.numprims - (splitidx - req.startidx), &node->rc, rightbounds, rightcentroid_bounds };
            
            // Put those to stack
            stack.push(leftrequest);
            stack.push(rightrequest);
        }
        
        // Set parent ptr if any
        if (req.ptr) *req.ptr = node;
    }
#else
    BuildNode(init, bounds, &centroids[0], &primids_[0]);
#endif
    
#ifdef USE_TBB
    taskgroup_.wait();
#endif
    
    // Set root_ pointer
    root_ = &nodes_[0];
}


bool Bvh::Intersect(ray& r, float& t, Intersection& isect) const
{
    // Check if we have been initialized
    assert(root_);
    // Maintain a stack of nodes to process
    std::stack<Node*> testnodes;
    // Precalc inv ray dir for bbox testing
    float3 invrd = float3(1.f / r.d.x, 1.f / r.d.y, 1.f / r.d.z);
    // Precalc ray direction signs: 1 if negative, 0 otherwise
    int dirneg[3] =
    {
        r.d.x < 0.f ? 1 : 0,
        r.d.y < 0.f ? 1 : 0,
        r.d.z < 0.f ? 1 : 0
    };
    // Current node
    Node* node = root_;
    // Hit flag
    bool hit = false;
    // Hit parametric distance
    float tt = r.t.y;
    // Start processing nodes
    // Changing the code to use more flow control
    // and skip push\pop when possible
    // This gives some perf boost
    for(;;)
    {
        if (node->type == kLeaf)
        {
            for (int i = node->startidx; i < node->startidx + node->numprims; ++i)
            {
                if (prims_[primids_[i]]->Intersect(r, tt, isect))
                {
                    hit = true;
                    r.t.y = tt;
                }
            }
        }
        else
        {
            bool addleft =  intersects(r, invrd, node->lc->bounds, dirneg);
            bool addright = intersects(r, invrd, node->rc->bounds, dirneg);
            
            if (addleft)
            {
                if (addright)
                {
                    testnodes.push(node->rc);
                }
                node = node->lc;
                continue;
            }
            else if (addright)
            {
                node = node->rc;
                continue;
            }
        }
        
        if (testnodes.empty())
        {
            break;
        }
        else
        {
            node = testnodes.top();
            testnodes.pop();
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
    // Precalc inv ray dir for bbox testing
    float3 invrd = float3(1.f / r.d.x, 1.f / r.d.y, 1.f / r.d.z);
    // Precalc ray direction signs: 1 if negative, 0 otherwise
    int dirneg[3] =
    {
        r.d.x < 0.f ? 1 : 0,
        r.d.y < 0.f ? 1 : 0,
        r.d.z < 0.f ? 1 : 0
    };
    // Current node
    Node* node = root_;
    // Start processing nodes
    // Changing the code to use more flow control
    // and skip push\pop when possible
    // This gives some perf boost
    for(;;)
    {
        if (node->type == kLeaf)
        {
            for (int i = node->startidx; i < node->startidx + node->numprims; ++i)
            {
                if (prims_[primids_[i]]->Intersect(r))
                {
                    return true;
                }
            }
        }
        else
        {
            bool addleft =  intersects(r, invrd, node->lc->bounds, dirneg);
            bool addright = intersects(r, invrd, node->rc->bounds, dirneg);
            
            if (addleft)
            {
                if (addright)
                {
                    testnodes.push(node->rc);
                }
                node = node->lc;
                continue;
            }
            else if (addright)
            {
                node = node->rc;
                continue;
            }
        }
        
        if (testnodes.empty())
        {
            break;
        }
        else
        {
            node = testnodes.top();
            testnodes.pop();
        }
    }
    
    return false;
}





