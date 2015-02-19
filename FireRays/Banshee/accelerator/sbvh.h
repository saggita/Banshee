/*
    Banshee and all code, documentation, and other materials contained
    therein are:

        Copyright 2013 Dmitry Kozlov
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
#ifndef SBVH_H
#define SBVH_H

#include "bvh.h"

///< BVH implementation which is based on SAH object/space splits 
///< Link: http://www.nvidia.com/object/nvidia_research_pub_012.html
///<
class Sbvh : public Bvh
{
public:
    // Pass triangle intersection cost compared to unit node traversal cost
    // 0 - no cost (intersection is free)
    // FLT_MAX - maximum cost (any intersection is heavier than any traversal)
    Sbvh(float trisah, int maxleafprims = 8,
         bool usespatial = true,
         int maxsptialdepth = 48,
         float minoverlap = 0.00001f)
        : trisah_(trisah)
        , maxleafprims_(maxleafprims)
        , usespatial_(usespatial)
        , maxspatialdepth_(maxsptialdepth)
        , minoverlap_(minoverlap)
    {
    }

protected:
    // Build function
    void BuildImpl(std::vector<Primitive*> const& prims);

private:
    // Struct to describe reference to a primitive
    struct PrimitiveRef
    {
        // Bounding box, which might encompass only a part of a primitive (split)
        bbox bounds;
        // Primitive index in the initial array
        int  idx;
        // Shortcut to the primitive for convenience (previous index might be used instead)
        // TODO: remove that later
        Primitive const* prim;
    };

    // Struct to describe a split
    struct Split
    {
        int   dim;
        float sah;
        float border;
        float overlaparea;
    };

    // Find best object split based on SAH
    Split FindObjectSplit(std::vector<PrimitiveRef> const& primrefs, int startidx, int numprims, bbox const& bounds, bbox const& centroid_bounds) const;
    // Find best spatial split based on SAH
    Split FindSpatialSplit(std::vector<PrimitiveRef> const& primrefs, int startidx, int numprims, bbox const& bounds, int depth) const;
    // Perform object split
    int   PerformObjectSplit(Split const& split, std::vector<PrimitiveRef>& primrefs, int startidx, int numprims) const;
    // Split primitive reference into 2 parts
    bool  SplitPrimRef(PrimitiveRef const& ref, int axis, float border, PrimitiveRef& leftref, PrimitiveRef& rightref) const;
    // Perform spatial split
    void  PerformSpatialSplit(Split const& split, std::vector<PrimitiveRef>& primrefs, int startidx, int numprims, int& idx, int& newnumprims) const;

    // Triangle intersection cost as a fraction of node traversal cost
    float trisah_;
    // Whether use spatial splits or not
    bool usespatial_;
    // Maximum allowed number of primitives in the leaf
    int maxleafprims_;
    // Maximum depth at which spatial split is allowed
    int maxspatialdepth_;
    // Minimum children overlap to consider a spatial split
    float minoverlap_;
};

#endif //SBVH_H