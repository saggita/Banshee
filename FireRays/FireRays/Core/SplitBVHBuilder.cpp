//
//  SplitBVHBuilder.cpp
//  FireRays
//
//  Created by dmitryk on 21.02.14.
//
//

#include "SplitBVHBuilder.h"

void SplitBVHBuilder::Build()
{
    maxLevel_ = 0;
    
    // Build the nodes over all primitive references
    BuildNode(GetBVH().GetPreRootNode(), BVH::ChildRel::CR_LEFT, refs_.begin(), refs_.end(), maxLevel_);
    
    //ReorderPrimitives();
    
    std::cout << "\n" << maxLevel_ << " levels in the tree";
}

BVH::NodeId SplitBVHBuilder::BuildNode(BVH::NodeId parentNode, BVH::ChildRel rel, PrimitiveRefIterator first, PrimitiveRefIterator last, unsigned level)
{
    // Check if we have a valid iterator range
    assert(std::distance(first, last) > 0);
    
    maxLevel_ = std::max(level, maxLevel_);
    
    // Start expanding form the first node bbox
    // and its center
    BBox nodesBbox = first->bbox;
    BBox nodeCentersBbox(nodesBbox.GetCenter());
    // calc number of nodes for later use
    unsigned primCount = 0;
    
    // Calculate bounding box of all the nodes
    // and all the node centers
    // The former is used to create the node itself
    // the latter is to choose principal direction to split along
    std::for_each(first, last, [&nodesBbox, &nodeCentersBbox, &primCount](PrimitiveRef const& info)
                  {
                      nodesBbox = BBoxUnion(nodesBbox, info.bbox);
                      nodeCentersBbox = BBoxUnion(nodeCentersBbox, info.bbox.GetCenter());
                      ++primCount;
                  }
                  );
    
    // Create leaf node if there is only one primitive
    if (primCount <= minPrimsPerLeaf_)
    {
        std::vector<unsigned> primIndices;
        std::for_each(first, last, [&primIndices](PrimitiveRef const& r)
                      {
                          primIndices.push_back(r.idx);
                      });
        
        return GetBVH().CreateLeafNode(parentNode, rel, nodesBbox, &primIndices[0], primIndices.size());
    }
    else
    {
        //  Split along a principal component of node centers
        int splitAxis = nodeCentersBbox.GetMaxDim();
        
        // Sort the list
        //{
        std::vector<PrimitiveRef> refs(first, last);
        
        last = refs_.erase(first, last);
        
        int refCount = refs.size();
        refs.resize(refCount * 2);
        
       // std::sort(refs.begin(), refs.begin() + refCount, [splitAxis](PrimitiveRef const& r1, PrimitiveRef const& r2)
                  //{
                     // return r1.bbox.GetCenter()[splitAxis] < r2.bbox.GetCenter()[splitAxis];
                  //});
        
        // Spatial split
        float splitValue = nodeCentersBbox.GetCenter()[splitAxis];
        int   splitNodeEnd = refCount;
        
        if (level < 20)
        {
            unsigned kNumTests = 10;
            float sahValue[10];

            float range = nodeCentersBbox.GetExtents()[splitAxis];
            float step  = range / (kNumTests + 1);

            for (unsigned i = 0; i < kNumTests; ++i)
            {
                BBox leftBox = BBox();
                BBox rightBox = BBox();
                unsigned leftCount(0), rightCount(0);

                float split = nodeCentersBbox.GetMinPoint()[splitAxis] + (i + 1)*step;

                for(int j = 0; j < refCount; ++j)
                {
                    PrimitiveRef r1, r2;
                    if (SplitPrimRef(refs[j], splitAxis, splitValue, r1, r2))
                    {
                        ++leftCount;
                        ++rightCount;
                        leftBox = BBoxUnion(leftBox, r1.bbox);
                        rightBox = BBoxUnion(rightBox, r2.bbox);
                    }
                    else
                    {
                        if (refs[j].bbox.GetCenter()[splitAxis] < split)
                        {
                            ++leftCount;
                            leftBox = BBoxUnion(leftBox, refs[j].bbox);
                        }
                        else
                        {
                            ++rightCount;
                            rightBox = BBoxUnion(rightBox, refs[j].bbox);

                        }
                    }
                }

                sahValue[i] = nodeSahCost_ + triSahCost_ * (leftCount * leftBox.GetSurfaceArea() + rightCount * rightBox.GetSurfaceArea())/nodesBbox.GetSurfaceArea();
            }

            float* minSah = std::min_element(sahValue, sahValue + kNumTests);
            unsigned minSahIdx = std::distance(sahValue, minSah);
            splitValue = nodeCentersBbox.GetMinPoint()[splitAxis] + (minSahIdx + 1) * step;

            for(int i = 0; i < refCount; ++i)
            {
                PrimitiveRef r1, r2;
                if (SplitPrimRef(refs[i], splitAxis, splitValue, r1, r2))
                {
                    refs[i] = r1;
                    refs[splitNodeEnd++] = r2;
                }
            }
        }
        
        auto split = std::partition(refs.begin(), refs.begin() + splitNodeEnd, [splitAxis, splitValue](PrimitiveRef const& r)
                                    {
                                        return r.bbox.GetCenter()[splitAxis] <= splitValue;
                                    }
                                    );
        
        int numSplitBefore = std::distance(refs.begin(), split);
        int numSplitAfter  = std::distance(split, refs.begin() + splitNodeEnd);
        
        first = refs_.insert(last, refs.begin(), refs.begin() + splitNodeEnd);
        
        
        auto splitList = first;
        
        if (numSplitBefore == 0 || numSplitAfter == 0)
        {
            std::advance(splitList, splitNodeEnd/2);
        }
        else
        {
            std::advance(splitList, numSplitBefore);
        }

        BVH::NodeId id = GetBVH().CreateInternalNode(parentNode, rel, static_cast<BVH::SplitAxis>(splitAxis), nodesBbox);

        BuildNode(id, BVH::ChildRel::CR_LEFT, first, splitList, ++level);
        BuildNode(id, BVH::ChildRel::CR_RIGHT, splitList, last, ++level);
        
        return id;
    }
}

BVH::NodeId SplitBVHBuilder::BuildNodeObjectSplitOnly(BVH::NodeId parentNode, BVH::ChildRel rel, PrimitiveRefIterator first, PrimitiveRefIterator last, unsigned level)
{
    // Check if we have a valid iterator range
    assert(std::distance(first, last) > 0);
    
    maxLevel_ = std::max(level, maxLevel_);
    
    // Start expanding form the first node bbox
    // and its center
    BBox nodesBbox = first->bbox;
    BBox nodeCentersBbox(nodesBbox.GetCenter());
    // calc number of nodes for later use
    unsigned primCount = 0;
    
    // Calculate bounding box of all the nodes
    // and all the node centers
    // The former is used to create the node itself
    // the latter is to choose principal direction to split along
    std::for_each(first, last, [&nodesBbox, &nodeCentersBbox, &primCount](PrimitiveRef const& info)
                  {
                      nodesBbox = BBoxUnion(nodesBbox, info.bbox);
                      nodeCentersBbox = BBoxUnion(nodeCentersBbox, info.bbox.GetCenter());
                      ++primCount;
                  }
                  );

    //std::cout << "Level " << maxLevel_ << " prim count" << primCount << "\n";
    
    // Create leaf node if there is only one primitive
    //  Split along a principal component of node centers
    int splitAxis = nodeCentersBbox.GetMaxDim();

    //std::cout << "BBOx: " << nodeCentersBbox.GetMinPoint()[splitAxis] << " " << nodeCentersBbox.GetMaxPoint()[splitAxis] << "\n";
    
    if (primCount < minPrimsPerLeaf_)
    {
        std::vector<unsigned> primIndices;
        std::for_each(first, last, [&primIndices](PrimitiveRef const& r)
                      {
                          primIndices.push_back(r.idx);
                      });
        
        return GetBVH().CreateLeafNode(parentNode, rel, nodesBbox, &primIndices[0], primIndices.size());
    }
    else
    {
        
        std::vector<PrimitiveRef> refs(first, last);
        
        std::sort(refs.begin(), refs.end(), [splitAxis](PrimitiveRef const& r1, PrimitiveRef const& r2)
                  {
                      return r1.bbox.GetCenter()[splitAxis] < r2.bbox.GetCenter()[splitAxis];
                  });

        unsigned splitIdx = primCount / 2;
        float splitSahValue = 1000.f;
        if (nodeCentersBbox.GetMinPoint()[splitAxis] != nodeCentersBbox.GetMaxPoint()[splitAxis])
        {
            // Spatial split
            float splitValue = nodeCentersBbox.GetCenter()[splitAxis];

            
            splitIdx =  FindObjectSplit(refs, splitAxis, nodesBbox, nodeCentersBbox, splitSahValue);
        }

        last = std::copy(refs.begin(), refs.end(), first);

        if ((splitSahValue > refs.size() * triSahCost_ && refs.size() < primsPerLeaf_))
        {
            std::vector<unsigned> primIndices;
            std::for_each(first, last, [&primIndices](PrimitiveRef const& r)
                          {
                              primIndices.push_back(r.idx);
                          });
            
            return GetBVH().CreateLeafNode(parentNode, rel, nodesBbox, &primIndices[0], primIndices.size());
        }
        else
        {
            auto splitList = first;
            
            std::advance(splitList, splitIdx);
            
            BVH::NodeId id = GetBVH().CreateInternalNode(parentNode, rel, static_cast<BVH::SplitAxis>(splitAxis), nodesBbox);
            
            BuildNodeObjectSplitOnly(id, BVH::ChildRel::CR_LEFT, first, splitList, ++level);
            BuildNodeObjectSplitOnly(id, BVH::ChildRel::CR_RIGHT, splitList, last, ++level);
            
            return id;
        }
    }
}

bool SplitBVHBuilder::SplitPrimRef(PrimitiveRef primRef, int splitAxis, float splitValue, PrimitiveRef& r1, PrimitiveRef& r2)
{
    if (((primRef.bbox.GetMinPoint()[splitAxis] <= splitValue) &&
         (primRef.bbox.GetMaxPoint()[splitAxis] <= splitValue)) ||
        ((primRef.bbox.GetMinPoint()[splitAxis] >= splitValue) &&
         (primRef.bbox.GetMaxPoint()[splitAxis] >= splitValue)))
        return false;
    
    // Assign initial values to r1 and r2
    // prim index is the same
    // and empty bboxes
    r1.idx = primRef.idx;
    r1.bbox = BBox();
    r2.idx = primRef.idx;
    r2.bbox = BBox();
    
    // Fetch the primitve
    Primitive prim = prims_[primRef.idx];
    
    // Split flag is set if the plane intersect the primitive
    bool split = false;
    
    // Iterate over the edges
    for (int i = 0; i < 3; ++i)
    {
        // Fetch positions
        vector3 v1 = positions_[prim.i[i]];
        vector3 v2 = positions_[prim.i[(i + 1)%3]];
        
        // Get the component we are interested in
        float vc1 = v1[splitAxis];
        float vc2 = v2[splitAxis];
        
        // Put the vertex into either r1 or
        if (vc1 < splitValue)
        {
            r1.bbox = BBoxUnion(r1.bbox, v1);
        }
        else
        {
            r2.bbox = BBoxUnion(r2.bbox, v1);
        }
        
        if ((vc1 < splitValue && vc2 > splitValue) ||
            (vc1 > splitValue && vc2 < splitValue))
        {
            split = true;
            
            // Interpolate the value
            float vct = std::min(std::max((splitValue - vc1) / (vc2 - vc1), 0.f), 1.f);
            // Find interpolated position
            vector3  vt = v1 * (1.f - vct) + v2 * vct;
            
            // Insert position into both bounding boxes
            r1.bbox = BBoxUnion(r1.bbox, vt);
            r2.bbox = BBoxUnion(r2.bbox, vt);
        }
    }
    
    // Intersect against the original bbox
    r1.bbox = BBoxIntersection(r1.bbox, primRef.bbox);
    r2.bbox = BBoxIntersection(r2.bbox, primRef.bbox);
    
    if (split)
    {
        //assert(Contains(primRef.bbox, r1.bbox));
        //assert(Contains(primRef.bbox, r2.bbox));
    }
    
    return split;
}

unsigned SplitBVHBuilder::FindObjectSplit(std::vector<PrimitiveRef>& refs, int splitAxis, BBox const& parentNodeBBox, BBox const& centroidsBBox,  float& splitSahValue)
{
    // SAH implementation
    // calc centroids histogram
    unsigned const kNumBins = 10;
    
    if (refs.size() < kNumBins)
    {
        return refs.size()/2;
    }
    
    struct Bin
    {
        BBox box;
        unsigned count;
    };
    
    Bin bins[kNumBins];
    
    float centroidRng = centroidsBBox.GetExtents()[splitAxis];
    float binRng = centroidRng / kNumBins;
    
    for (unsigned i = 0; i < kNumBins; ++i)
    {
        bins[i].count = 0;
        bins[i].box = BBox();
    }
    
    for (unsigned i = 0; i < refs.size(); ++i)
    {
        unsigned bin_idx = (unsigned)std::min<float>(kNumBins * ((refs[i].bbox.GetCenter()[splitAxis] - centroidsBBox.GetMinPoint()[splitAxis]) / centroidRng), kNumBins-1);
        
        assert(bin_idx >= 0);
        
        ++bins[bin_idx].count;
        bins[bin_idx].box = BBoxUnion(bins[bin_idx].box, refs[i].bbox);
    }
    
    float sahCost[kNumBins-1];
    
    for (unsigned i = 0; i < kNumBins - 1; ++i)
    {
        BBox h1Box = BBox();
        unsigned h1Count = 0;
        
        for(unsigned j = 0; j <= i; ++j)
        {
            h1Box = BBoxUnion(h1Box, bins[j].box);
            h1Count += bins[j].count;
        }
        
        BBox h2Box = BBox();
        unsigned h2Count = 0;
        
        for(unsigned j = i + 1; j < kNumBins; ++j)
        {
            h2Box = BBoxUnion(h2Box, bins[j].box);
            h2Count += bins[j].count;
        }
        
        sahCost[i] = nodeSahCost_ + triSahCost_ * (h1Count * h1Box.GetSurfaceArea() + h2Count * h2Box.GetSurfaceArea())/parentNodeBBox.GetSurfaceArea();
    }
    
    float* minSahCost = std::min_element(sahCost, sahCost + kNumBins - 1);
    
    splitSahValue = *minSahCost;
    
    unsigned splitIdx = std::distance(sahCost, minSahCost);
    
    float border = bins[splitIdx + 1].box.GetCenter()[splitAxis];
    auto iter = std::partition(refs.begin(), refs.end(), [=](PrimitiveRef const& r)
                               {
                                   return r.bbox.GetCenter()[splitAxis] < border;
                               });
    unsigned si = std::distance(refs.begin(), iter);

    return si;
}

unsigned         SplitBVHBuilder::GetPrimitiveCount() const
{
    return prims_.size();
}

SplitBVHBuilder::Primitive const* SplitBVHBuilder::GetPrimitives() const
{
    return &prims_[0];
}

void SplitBVHBuilder::ReorderPrimitives()
{
    std::vector<Primitive> tempPrims(prims_.begin(), prims_.end());
    
    unsigned idxCount = bvh_->GetPrimitiveIndexCount();
    unsigned const* indices = bvh_->GetPrimitiveIndices();
    
    
    for( int i = 0; i < idxCount; ++i)
    {
        prims_[i] = tempPrims[indices[i]];
    }
}
