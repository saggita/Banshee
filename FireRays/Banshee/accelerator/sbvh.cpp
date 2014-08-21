#include "sbvh.h"

#include "../math/mathutils.h"

#include <cassert>
#include <stack>
#include <numeric>
#include <iostream>

// Build function
void Sbvh::BuildImpl(std::vector<Primitive*> const& prims)
{
    // Count splits
    int numobjsplits = 0;
    int numspatialsplits = 0;

    // Structure describing split request
    struct SplitRequest
    {
        int startidx;
        int numprims;
        Node** ptr;
    };

    // We use indices as primitive references
    std::vector<PrimitiveRef> primrefs(2*prims.size());
    for (int i = 0; i < prims.size(); ++i)
    {
        primrefs[i].bounds = prims[i]->Bounds();
        primrefs[i].idx = i;
    }

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

        // Calc bounds for primitives and centroids
        bbox centroid_bounds = bbox();
        for (int i = req.startidx; i < req.startidx + req.numprims; ++i)
        {
            node->bounds = bboxunion(node->bounds, primrefs[i].bounds);
            centroid_bounds = bboxunion(centroid_bounds, primrefs[i].bounds.center());
        }

        // Create leaf node if we have only a single prim
        if (req.numprims < 2)
        {
            node->type = kLeaf;
            node->startidx = (int)primitives_.size();
            node->numprims = req.numprims;
            for (int i = req.startidx; i < req.startidx + req.numprims; ++i)
            {
                primitives_.push_back(prims[primrefs[i].idx]);
            }
        }
        else
        {
            // Find best object split
            Split objsplit = FindObjectSplit(primrefs, req.startidx, req.numprims, node->bounds, centroid_bounds);
            // Find best spatial split
            Split spatialsplit = FindSpatialSplit(primrefs, req.startidx, req.numprims, node->bounds);
            // Calculate leaf creation cost: FLT_MAX if we cannot create leaf
            float leafsah = req.numprims < maxleafprims_ ? (req.numprims * trisah_) : std::numeric_limits<float>::max(); 
            float minsah = std::min(leafsah, std::min(spatialsplit.sah, objsplit.sah));

            if (minsah == objsplit.sah)
            {
                node->type = kInternal;
                ++numobjsplits;
                int idx = PerformObjectSplit(objsplit, primrefs, req.startidx, req.numprims);

                // Left request
                SplitRequest leftrequest = {req.startidx, idx, &node->lc};
                // Right request
                SplitRequest rightrequest = {req.startidx + idx, req.numprims - idx,  &node->rc};

                // Put those to stack
                stack.push(leftrequest);
                stack.push(rightrequest);
            }
            else if (minsah == spatialsplit.sah)
            {
                node->type = kInternal;
                ++numspatialsplits;
                int idx = 0;
                int newnumprims = req.numprims;

                // Perform spatial split
                PerformSpatialSplit(spatialsplit, primrefs, req.startidx, req.numprims, idx, newnumprims);
                // Left request
                SplitRequest leftrequest = {req.startidx, idx, &node->lc};
                // Right request
                SplitRequest rightrequest = {req.startidx + idx, newnumprims - idx,  &node->rc};

                // Put those to stack
                // ATTENTION!!!: the order is critical here as we are overwriting values in primrefs array following
                // the right end of a current request, so important to handle right then left
                stack.push(leftrequest);
                stack.push(rightrequest);
            }
            else
            {
                // Create leaf node
                node->type = kLeaf;
                node->startidx = (int)primitives_.size();
                node->numprims = req.numprims;
                for (int i = req.startidx; i < req.startidx + req.numprims; ++i)
                {
                    primitives_.push_back(prims[primrefs[i].idx]);
                }
            }
        }

        // Set parent ptr if any
        if (req.ptr) *req.ptr = node;
    }
    
    // Set root_ pointer
    root_ = nodes_[0].get();

    // Output debug data
    std::cout << "Sbvh: number of object splits = " << numobjsplits << "\n";
    std::cout << "Sbvh: number of spatial splits = " << numspatialsplits << "\n";
}


Sbvh::Split Sbvh::FindObjectSplit(std::vector<PrimitiveRef> const& primrefs, int startidx, int numprims, bbox const& bounds, bbox const& centroid_bounds) const
{
    Split split;
    // SAH implementation
    // calc centroids histogram
    int const kNumBins = 16;
    // moving split bin index
    int splitidx = -1;
    // Set SAH to maximum float value as a start
    split.dim = 0;
    split.sah = std::numeric_limits<float>::max();
    split.border = std::numeric_limits<float>::quiet_NaN();

    // if we cannot apply histogram algorithm
    // put NAN sentinel as split border
    // PerformObjectSplit simply splits in half
    // in this case
    float3 extents = bounds.extents();
    float3 centroid_extents = centroid_bounds.extents();
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
    float invarea = 1.f / bounds.surface_area();
    // Precompute min point
    float3 rootmin = centroid_bounds.pmin;

    // Evaluate all dimensions
    for(int axis = 0; axis < 3; ++axis)
    {
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
        for (int i = startidx; i < startidx + numprims; ++i)
        {
            PrimitiveRef const& primref(primrefs[i]);
            int binidx = (int)std::min<float>(kNumBins * ((primref.bounds.center()[axis] - rootmin[axis]) * invcentroid_rng), kNumBins-1);
            
            assert(binidx >= 0);
            
            ++bins[axis][binidx].count;
            bins[axis][binidx].bounds = bboxunion(bins[axis][binidx].bounds, primref.bounds);
        }

        bbox rightbounds[kNumBins-1];

        // Start with 1-bin right box
        bbox rightbox = bbox();
        for (int i = kNumBins - 1; i > 0; --i)
        {
            rightbox = bboxunion(rightbox, bins[axis][i].bounds);
            rightbounds[i-1] = rightbox;
        }

        bbox leftbox = bbox();
        int  leftcount = 0;
        int  rightcount = numprims;

        // Start best SAH search
        // i is current split candidate (split between i and i + 1) 
        for (int i = 0; i < kNumBins - 1; ++i)
        {
            leftbox = bboxunion(leftbox, bins[axis][i].bounds);
            leftcount += bins[axis][i].count;
            rightcount -= bins[axis][i].count;

            // Compute SAH
            float sah = 1.f + trisah_ * (leftcount * leftbox.surface_area() + rightcount * rightbounds[i].surface_area()) * invarea;

            // Check if it is better than what we found so far
            if (sah < split.sah)
            {
                split.dim = axis;
                splitidx = i;
                split.sah = sah;
            }
        }
    }

    // Choose split plane
    if (splitidx != -1)
    {
        split.border = rootmin[split.dim] + (splitidx + 1) * (centroid_extents[split.dim] / kNumBins);  
    }

    return split;
}


int   Sbvh::PerformObjectSplit(Split const& split, std::vector<PrimitiveRef>& primrefs, int startidx, int numprims) const
{
    int idx = 0;
    // Check for NaN sentinel, it is set by FindObjectSplit when no SAH estiamtes are possible
    if (!is_nan(split.border))
    {
        // Partition into two sets
        auto part = std::partition(primrefs.begin() + startidx, primrefs.begin() + startidx + numprims, [&](PrimitiveRef const& primref)
        {
            bbox b = primref.bounds;
            return b.center()[split.dim] < split.border;
        }
        );

        // Find split index relative to req.startidx
        idx = (int)(part - (primrefs.begin() + startidx));
    }

    // If we have not split anything use split in halves
    if (idx == 0
        || idx == numprims)
    {
        idx = numprims >> 1;
    }

    return idx;
}

bool  Sbvh::SplitPrimRef(PrimitiveRef const& ref, int axis, float border, PrimitiveRef& leftref, PrimitiveRef& rightref) const
{
    if (border < ref.bounds.pmax[axis] && border > ref.bounds.pmin[axis])
    {
        // Copy min and max values
        leftref.bounds.pmin = rightref.bounds.pmin = ref.bounds.pmin;
        leftref.bounds.pmax = rightref.bounds.pmax = ref.bounds.pmax;
        leftref.idx = rightref.idx = ref.idx;

        // Break now
        leftref.bounds.pmax[axis] = border;
        rightref.bounds.pmin[axis] = border;

        return true;
    }

    return false;
}

Sbvh::Split Sbvh::FindSpatialSplit(std::vector<PrimitiveRef> const& primrefs, int startidx, int numprims, bbox const& bounds) const
{
    // Split structure
    Split split;
    // Calculate kNumBins-histogram
    int const kNumBins = 16;
    // Set SAH to maximum float value as a start
    split.dim = 0;
    split.sah = std::numeric_limits<float>::max();
    split.border = std::numeric_limits<float>::quiet_NaN();
    // Extents
    float3 extents = bounds.extents();
 
    // If there are too few primitives don't split them
    if (numprims < kNumBins || (extents.sqnorm() == 0.f) || !usespatial_ || numprims > maxspatial_)
    {
        return split;
    }

    // Bin has start and exit counts + bounds
    struct Bin
    {
        bbox bounds;
        int enter;
        int exit;
    };
    
    Bin bins[3][kNumBins];

    // Prepcompute some useful stuff
    float3 origin  = bounds.pmin;
    float3 binsize = bounds.extents() * (1.f / kNumBins);
    float3 invbinsize = float3(1.f / binsize.x, 1.f / binsize.y, 1.f / binsize.z);

    // Initialize bins
    for (int axis = 0; axis < 3; ++axis)
    {
        for (int i = 0; i < kNumBins; ++i)
        {
            bins[axis][i].bounds = bbox();
            bins[axis][i].enter = 0;
            bins[axis][i].exit = 0;
        }
    }

    // Iterate thru all primitive refs
    for (int i = startidx; i < startidx + numprims; ++i)
    {
        PrimitiveRef const& primref(primrefs[i]);
        // Determine starting bin for this primitive
        float3 firstbin = clamp((primref.bounds.pmin - origin) * invbinsize, float3(0,0,0), float3(kNumBins-1, kNumBins-1, kNumBins-1));
        // Determine finishing bin
        float3 lastbin = clamp((primref.bounds.pmin - origin) * invbinsize, firstbin, float3(kNumBins-1, kNumBins-1, kNumBins-1));
        // Iterate over axis
        for (int axis = 0; axis < 3; ++axis)
        {
            // Skip in case of a degenerate dimension
            if (extents[axis] == 0.f) continue;
            // Break the prim into bins
            PrimitiveRef tempref = primref;
            for (int j = (int)firstbin[axis]; j < (int)lastbin[axis]; ++j)
            {
                PrimitiveRef leftref, rightref;
                // Split primitive ref into left and right
                SplitPrimRef(tempref, axis, origin[axis] + binsize[axis] * (j + 1), leftref, rightref);
                // Add left one
                bins[axis][j].bounds = bboxunion(bins[axis][j].bounds, leftref.bounds);
                // Save right to add part of it into the next bin
                tempref = rightref;
            }
            // Add the last piece into the last bin
            bins[axis][(int)lastbin[axis]].bounds = bboxunion(bins[axis][(int)lastbin[axis]].bounds, tempref.bounds);
            // Adjust enter & exit counters
            bins[axis][(int)firstbin[axis]].enter++;
            bins[axis][(int)lastbin[axis]].exit++;
        }
    }

    // Prepare moving window data
    bbox rightbounds[kNumBins - 1];
    split.sah = std::numeric_limits<float>::max();

    // Iterate over axis
    for (int axis = 0; axis < 3; ++axis)
    {
        // Skip if the extent is degenerate in that direction
        if (extents[axis] == 0.f)
            continue;

        // Start with 1-bin right box
        bbox rightbox = bbox();
        for (int i = kNumBins - 1; i > 0; --i)
        {
            rightbox = bboxunion(rightbox, bins[axis][i].bounds);
            rightbounds[i-1] = rightbox;
        }

        bbox leftbox = bbox();
        int  leftcount = 0;
        int  rightcount = numprims;

        // Start moving border to the right
        for (int i = 1; i < kNumBins; ++i)
        {
            // New left box
            leftbox = bboxunion(leftbox, bins[axis][i-1].bounds);
            // New left box count
            leftcount += bins[axis][i-1].enter;
            // Adjust right box
            rightcount -= bins[axis][i-1].exit;
            // Calc SAH
            float sah = 1.f + trisah_ * (leftbox.surface_area() * leftcount  + rightbounds[i-1].surface_area() * rightcount) / bounds.surface_area();

            // Update SAH if it is needed
            if (sah < split.sah)
            {
                split.sah = sah;
                split.dim = axis;
                split.border = origin[axis] + binsize[axis] * (float)i;
            }
        }
    }

    return split;
}

 // Perform spatial split
void   Sbvh::PerformSpatialSplit(Split const& split, std::vector<PrimitiveRef>& primrefs, int startidx, int numprims, int& idx, int& newnumprims) const
{
    // We are going to append new primitives at the end of the array
    int appendprims = numprims;

    // Check for the sentinel and split in halves if it is set
    if (!is_nan(split.border))
    {
        // Split refs if any of them require to be split
        for (int i = startidx; i != startidx + numprims; ++i)
        {
            PrimitiveRef leftref, rightref;
            if (SplitPrimRef(primrefs[i], split.dim, split.border, leftref, rightref))
            {
                // Copy left ref instead of original
                primrefs[i] = leftref;
                // Append right one at the end
                primrefs[startidx + appendprims++] = rightref;
            }
        }

        // Partition the whole set using split border
        auto part = std::partition(primrefs.begin() + startidx, primrefs.begin() + startidx + appendprims, [&](PrimitiveRef const& primref)
        {
            bbox b = primref.bounds;
            return b.center()[split.dim] < split.border;
        }
        );

        // TODO: add unsplit handling

        // Find split index relative to startidx
        idx = (int)(part - (primrefs.begin() + startidx));
    }

    // If we have not split anything use split in halves
    if (idx == 0 || idx == appendprims)
    {
        idx = numprims >> 1;
    }

    // Return number of primitives after this operation
    newnumprims = appendprims;
}
