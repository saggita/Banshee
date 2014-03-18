//
//  SplitBVHBuilder.cpp
//  FireRays
//
//  Created by dmitryk on 21.02.14.
//
//

#include "SplitBVHBuilder.h"

#include "utils.h"
#include <cmath>


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
    // Remove empty refs
    last = RemoveEmptyRefs(first, last);
    
    // Check if we have a valid iterator range
    unsigned primCount = std::distance(first, last);
    
    assert( primCount > 0);
    
    maxLevel_ = std::max(level, maxLevel_);
    
    std::vector<PrimitiveRef> refs(first, last);
        
    last = refs_.erase(first, last);

    NodeDesc desc;
    CreateNodeDesc(refs.begin(), refs.end(), desc);
    
    SplitDesc objectSplit;
    FindObjectSplit(desc, refs.begin(), refs.end(), objectSplit);
    
    SplitDesc spatialSplit;
    FindSpatialSplit(desc, refs.begin(), refs.end(), spatialSplit);
    
    float leafSah = primCount * triSahCost_;
    
    float minSah = std::min(std::min(leafSah, objectSplit.sah), spatialSplit.sah);
    
    if ( (minSah == leafSah && primCount < primsPerLeaf_) || primCount == 1)
    {
        
        std::vector<unsigned> primIndices;
        std::for_each(refs.begin(), refs.end(), [&primIndices](PrimitiveRef const& r)
                      {
                          primIndices.push_back(r.idx);
                      });
        
        return GetBVH().CreateLeafNode(parentNode, rel, desc.bbox, &primIndices[0], primIndices.size());
    }
    else if (spatialSplit.sah < objectSplit.sah)
    {
        BVH::NodeId id = GetBVH().CreateInternalNode(parentNode, rel, static_cast<BVH::SplitAxis>(objectSplit.dim), desc.bbox);
        
        refs.resize(primCount * 2);
        
        unsigned newPrimCount;
        
        auto splitIter = PerformSpatialSplit(refs.begin(), refs.begin() + primCount, spatialSplit, newPrimCount);
        
        auto dist = std::distance(refs.begin(), splitIter);
        
        first = refs_.insert(last, refs.begin(), refs.begin() + newPrimCount);
        
        auto splitListIter = first;
        std::advance(splitListIter, dist);
        
        BuildNode(id, BVH::ChildRel::CR_LEFT,  first, splitListIter, ++level);
        BuildNode(id, BVH::ChildRel::CR_RIGHT, splitListIter, last, ++level);
    }
    else
    {
        BVH::NodeId id = GetBVH().CreateInternalNode(parentNode, rel, static_cast<BVH::SplitAxis>(objectSplit.dim), desc.bbox);

        auto splitIter = PerformObjectSplit(refs.begin(), refs.end(), objectSplit);
        auto dist = std::distance(refs.begin(), splitIter);
        
        first = refs_.insert(last, refs.begin(), refs.end());
        
        auto splitListIter = first;
        std::advance(splitListIter, dist);
        
        BuildNode(id, BVH::ChildRel::CR_LEFT,  first, splitListIter, ++level);
        BuildNode(id, BVH::ChildRel::CR_RIGHT, splitListIter, last, ++level);
        
        return id;
    }
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

// Find best object split according to 10-bins histogram SAH
void SplitBVHBuilder::FindObjectSplit(NodeDesc const& desc, std::vector<PrimitiveRef>::iterator begin, std::vector<PrimitiveRef>::iterator end, SplitDesc& split)
{
    // SAH implementation
    // calc centroids histogram
    unsigned const kNumBins = 64;
    // moving split bin index
    unsigned splitIdx = 0;
    
    split.lb = BBox();
    split.rb = BBox();
    split.sah = std::numeric_limits<float>::max();
    
    // if we cannot apply histogram algorithm
    // put NAN sentinel as split val
    // PerformObjectSplit simply splits in half
    // in this case
    if (desc.cbox.GetExtents().x() == 0 &&
        desc.cbox.GetExtents().y() == 0 &&
        desc.cbox.GetExtents().z() == 0)
    {
        split.val = NAN;
        split.sah = NAN;
        return;
    }
    
    // Bin has bbox and occurence count
    struct Bin
    {
        BBox box;
        unsigned count;
    };
    
    // Keep bins for each dimension
    Bin   bins[3][kNumBins];
    
    for(int axis = 0; axis < 3; ++axis)
    {
        float centroidRng = desc.cbox.GetExtents()[axis];
        
        for (unsigned i = 0; i < kNumBins; ++i)
        {
            bins[axis][i].count = 0;
            bins[axis][i].box = BBox();
        }
        
        for (auto i = begin; i < end; ++i)
        {
            unsigned binIdx = (unsigned)std::min<float>(kNumBins * ((i->bbox.GetCenter()[axis] - desc.cbox.GetMinPoint()[axis]) / centroidRng), kNumBins-1);
            
            assert(binIdx >= 0);
            
            ++bins[axis][binIdx].count;
            bins[axis][binIdx].box = BBoxUnion(bins[axis][binIdx].box, i->bbox);
        }
        
        for (unsigned i = 0; i < kNumBins - 1; ++i)
        {
            BBox h1Box = BBox();
            unsigned h1Count = 0;
            
            for(unsigned j = 0; j <= i; ++j)
            {
                h1Box = BBoxUnion(h1Box, bins[axis][j].box);
                h1Count += bins[axis][j].count;
            }
            
            BBox h2Box = BBox();
            unsigned h2Count = 0;
            
            for(unsigned j = i + 1; j < kNumBins; ++j)
            {
                h2Box = BBoxUnion(h2Box, bins[axis][j].box);
                h2Count += bins[axis][j].count;
            }
            
            float sah = nodeSahCost_ + triSahCost_ * (h1Count * h1Box.GetSurfaceArea() + h2Count * h2Box.GetSurfaceArea())/desc.bbox.GetSurfaceArea();
            
            if (sah < split.sah)
            {
                split.dim = axis;
                splitIdx = i;
                split.sah = sah;
            }
        }
    }
    
    // Choose split plane
    split.val = bins[split.dim][splitIdx + 1].box.GetCenter()[split.dim];
}

// Find best spatial split according to 10-bins histogram SAH
void SplitBVHBuilder::FindSpatialSplit(NodeDesc const& desc, std::vector<PrimitiveRef>::iterator begin, std::vector<PrimitiveRef>::iterator end, SplitDesc& split)
{
    unsigned primCount = std::distance(begin, end);
    
    int const kNumBins = 64;
    
    if (primCount < kNumBins)
    {
        split.sah = NAN;
        split.val = NAN;
        return;
    }

    struct Bin
    {
        BBox bbox;
        unsigned enter;
        unsigned exit;
    };
    
    Bin bins[3][kNumBins];
    
    // Initialize bins.
    vector3 origin  = desc.bbox.GetMinPoint();
    vector3 binSize = desc.bbox.GetExtents() * (1.f / kNumBins);
    vector3 invBinSize = vector3(1.f / binSize.x(), 1.f / binSize.y(), 1.f / binSize.z());
    
    char dims[3] = {0, 0, 0};
    unsigned dimCount = 0;
    
    for (int dim = 0; dim < 3; ++dim)
    {
        if (binSize[dim] != 0.f)
            dims[dimCount++] = dim;
    }
    
    for (int dim = 0; dim < dimCount; dim++)
    {
        for (int i = 0; i < kNumBins; i++)
        {
            bins[dims[dim]][i].bbox = BBox();
            bins[dims[dim]][i].enter = 0;
            bins[dims[dim]][i].exit = 0;
        }
    }
    
    for (auto iter = begin; iter != end; ++iter)
    {
        vector3 firstBin = clamp(vector3((iter->bbox.GetMinPoint() - origin) * invBinSize), vector3(0,0,0),  vector3(kNumBins - 1, kNumBins - 1, kNumBins - 1));
        vector3 lastBin = clamp(vector3((iter->bbox.GetMaxPoint() - origin) * invBinSize), firstBin, vector3(kNumBins - 1, kNumBins - 1, kNumBins - 1));
        
        for (int dim = 0; dim < dimCount; dim++)
        {
            PrimitiveRef ref = *iter;
            for (int i = (int)firstBin[dims[dim]]; i < (int)lastBin[dims[dim]]; i++)
            {
                PrimitiveRef leftRef, rightRef;
                SplitPrimRef(ref, dims[dim], origin[dims[dim]] + binSize[dims[dim]] * (float)(i + 1), leftRef, rightRef);
                
                bins[dims[dim]][i].bbox = BBoxUnion(bins[dims[dim]][i].bbox, leftRef.bbox);
                ref = rightRef;
            }
            
            bins[dims[dim]][(int)lastBin[dims[dim]]].bbox = BBoxUnion(bins[dims[dim]][(int)lastBin[dims[dim]]].bbox, ref.bbox);
            bins[dims[dim]][(int)firstBin[dims[dim]]].enter++;
            bins[dims[dim]][(int)lastBin[dims[dim]]].exit++;
        }
    }
    
    BBox rightBounds[kNumBins - 2];
    
    split.sah = std::numeric_limits<float>::max();
    
    for (int dim = 0; dim < dimCount; dim++)
    {
        
        BBox rb = BBox();
        for (int i = kNumBins - 1; i > 0; i--)
        {
            rb = BBoxUnion(rb, bins[dims[dim]][i].bbox);
            rightBounds[i - 1] = rb;
        }
        
        BBox lb = BBox();
        int leftNum = 0;
        int rightNum = primCount;
        
        for (int i = 1; i < kNumBins; i++)
        {
            lb = BBoxUnion(lb, bins[dims[dim]][i - 1].bbox);
            leftNum += bins[dims[dim]][i - 1].enter;
            rightNum -= bins[dims[dim]][i - 1].exit;
            
            float sah = nodeSahCost_ + triSahCost_ * (lb.GetSurfaceArea() * leftNum  + rightBounds[i - 1].GetSurfaceArea() * rightNum) / desc.bbox.GetSurfaceArea();
            
            if (sah < split.sah)
            {
                split.sah = sah;
                split.dim = dims[dim];
                split.val = origin[dims[dim]] + binSize[dims[dim]] * (float)i;
            }
        }
    }
}

void SplitBVHBuilder::CreateNodeDesc(std::vector<PrimitiveRef>::iterator begin, std::vector<PrimitiveRef>::iterator end, NodeDesc& desc)
{
    desc.bbox = BBox();
    desc.cbox = BBox();
    desc.begin = begin;
    desc.end = end;
    
    std::for_each(begin, end, [&desc](PrimitiveRef const& info)
                  {
                      desc.bbox = BBoxUnion(desc.bbox, info.bbox);
                      desc.cbox = BBoxUnion(desc.cbox, info.bbox.GetCenter());
                  }
                  );
}

// Perform object split
std::vector<SplitBVHBuilder::PrimitiveRef>::iterator SplitBVHBuilder::PerformObjectSplit(std::vector<PrimitiveRef>::iterator begin, std::vector<PrimitiveRef>::iterator end, SplitDesc const& splitDesc)
{
    // Check for NaN sentinel and split in half in this case
    if (std::isnan(splitDesc.val))
    {
        std::advance(begin, std::distance(begin, end) / 2);
        return begin;
    }
    else
    {
        // Partition according to splitting plane otherwise
        return std::partition(begin, end, [=](PrimitiveRef const& r)
                              {
                                  return r.bbox.GetCenter()[splitDesc.dim] < splitDesc.val;
                              });
    }
}

SplitBVHBuilder::PrimitiveRefIterator SplitBVHBuilder::RemoveEmptyRefs(PrimitiveRefIterator begin, PrimitiveRefIterator end)
{
    return std::remove_if(begin, end, [](PrimitiveRef const& r)
                          {
                              return r.bbox.GetSurfaceArea() == 0.f;
                          }
                          );
}

bool SplitBVHBuilder::SplitPrimRef(PrimitiveRef primRef, int splitAxis, float splitValue, PrimitiveRef& r1, PrimitiveRef& r2)
{
//    if (((primRef.bbox.GetMinPoint()[splitAxis] <= splitValue) &&
//         (primRef.bbox.GetMaxPoint()[splitAxis] <= splitValue)) ||
//        ((primRef.bbox.GetMinPoint()[splitAxis] >= splitValue) &&
//         (primRef.bbox.GetMaxPoint()[splitAxis] >= splitValue)))
//        return false;
    
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
        if (vc1 <= splitValue)
        {
            r1.bbox = BBoxUnion(r1.bbox, v1);
        }
        
        if (vc1 >= splitValue)
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

std::vector<SplitBVHBuilder::PrimitiveRef>::iterator SplitBVHBuilder::PerformSpatialSplit(std::vector<PrimitiveRef>::iterator begin, std::vector<PrimitiveRef>::iterator end, SplitDesc const& splitDesc, unsigned& newPrimCount)
{
    auto newPrimsIter = end;
    for (auto iter = begin; iter != end; ++iter)
    {
        PrimitiveRef r1, r2;
        if (SplitPrimRef(*iter, splitDesc.dim, splitDesc.val, r1, r2))
        {
            *iter = r1;
            *newPrimsIter = r2;
            ++newPrimsIter;
        }
    }
    
    newPrimCount = std::distance(begin, newPrimsIter);
    
    return std::partition(begin, newPrimsIter, [=](PrimitiveRef const& r)
                                {
                                    return r.bbox.GetCenter()[splitDesc.dim] <= splitDesc.val;
                                }
                                );
}

