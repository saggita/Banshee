//
//  BVHAccelerator.cpp
//  BVHOQ
//
//  Created by dmitryk on 09.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#include "BVHAccelerator.h"

#include <algorithm>

bool Intersect(ray const& r, BBox const& box, float& t)
{
    float tmin, tmax, tymin, tymax, tzmin, tzmax;
    vector3 Bounds[2] = { box.GetMinPoint(), box.GetMaxPoint() };

    if (r.d.x() >= 0) {
        tmin = (Bounds[0].x() - r.o.x()) / r.d.x();
        tmax = (Bounds[1].x() - r.o.x()) / r.d.x();
    } else {
        tmin = (Bounds[1].x() - r.o.x()) / r.d.x();
        tmax = (Bounds[0].x() - r.o.x()) / r.d.x();
    }
    if (r.d.y() >= 0) {
        tymin = (Bounds[0].y() - r.o.y()) / r.d.y();
        tymax = (Bounds[1].y() - r.o.y()) / r.d.y();
    } else {
        tymin = (Bounds[1].y() - r.o.y()) / r.d.y();
        tymax = (Bounds[0].y() - r.o.y()) / r.d.y();
    }
    if ( (tmin > tymax) || (tymin > tmax) )
        return false;

    if (tymin > tmin)
        tmin = tymin;
    if (tymax < tmax)
        tmax = tymax;
    if (r.d.z() >= 0) {
        tzmin = (Bounds[0].z() - r.o.z()) / r.d.z();
        tzmax = (Bounds[1].z() - r.o.z()) / r.d.z();
    } else {
        tzmin = (Bounds[1].z() - r.o.z()) / r.d.z();
        tzmax = (Bounds[0].z() - r.o.z()) / r.d.z();
    }
    if ( (tmin > tzmax) || (tzmin > tmax) )
        return false;

    if (tzmin > tmin)
        tmin = tzmin;
    if (tzmax < tmax)
        tmax = tzmax;

    t = tmin;

    if (t < 0 && tmax < 0) return  false;
    else t = tmax;

    return true;
}

bool Intersect(ray const& r, vector3 const& v1, vector3 const& v2, vector3 const& v3, float& t)
{
    vector3 e1 = v2 - v1;
    vector3 e2 = v3 - v1;
    vector3 s1 = cross(r.d, e2);
    float div = dot(s1, e1);

    if (div == 0.f)
        return false;

    vector3 d = r.o - v1;
    float b1 = dot(d,s1) / div;

    if (b1 < 0 || b1 > 1)
        return false;

    vector3 s2 = cross(d, e1);
    float b2 = dot(r.d, s2) / div;

    if (b2 < 0 || b1 + b2 > 1)
        return false;

    t = dot(e2, s2) / div;

    if (t < 0)
    {
        return false;
    }

    return true;
}

float SurfaceArea(vector3 const& v1, vector3 const& v2, vector3 const& v3)
{
    vector3 v12 = v2 - v1;
    vector3 v13 = v3 - v1;

    return 0.5f * sqrtf(cross(v12, v13).sq_norm());

}

template <typename T> BBox BVHAccelerator::CalcBbox(Triangle const& t, std::vector<T> const& vertices)
{
    BBox b(vertices[t.i1].position, vertices[t.i2].position);
    return BBoxUnion(b, vertices[t.i3].position);
}

std::shared_ptr<BVHAccelerator> BVHAccelerator::CreateFromScene(SceneBase const& scene)
{
    return std::shared_ptr<BVHAccelerator>
        (new BVHAccelerator(scene.GetVertices(), scene.GetVertexCount(), scene.GetIndices(), scene.GetIndexCount(), scene.GetMaterials(), 8U));
}

template <typename T> BVHAccelerator::BVHAccelerator(T const* vertices, unsigned int vertexCount, unsigned const* indices, unsigned indexCount, unsigned const* materials, unsigned maxPrimsPerNode)
    : maxNodePrims_(maxPrimsPerNode)
{
    /// build primitives list
    prims_.resize(indexCount/3);
    buildInfo_.resize(indexCount/3);
    for (unsigned i = 0; i < indexCount; i += 3)
    {
        BBox b(vertices[indices[i]].position, vertices[indices[i+1]].position);
        b = BBoxUnion(b, vertices[indices[i+2]].position);

        Triangle t = {indices[i], indices[i + 1], indices[i + 2], materials[i/3]};
        prims_[i/3] = t;

        BuildInfo info = { b, i/3 };
        buildInfo_[i/3] = info;
    }

    std::vector<BuildNode> buildNodes;
    root_ = BuildHierarchy(0, (unsigned)buildInfo_.size(), buildNodes);

    /// this is important to avoid nodes_ reallocation
    /// while recursing down the structure
    nodes_.reserve(buildNodes.size());
    Linearize(root_, 0, buildNodes);
    std::cout << "BVH Contains " << nodes_.size() << " nodes\n";
    
    
    Traverse(root_, buildNodes, bvh_.GetPreRootNode(), BVH::CR_LEFT/*this arg doesn't matter*/);
}


BVHAccelerator::~BVHAccelerator()
{
}

unsigned BVHAccelerator::CreateLeafNode(std::vector<BuildNode>& buildNodes, unsigned begin, unsigned end, BBox const& b)
{
    // reorder primitives to have only start ptr and size in a leaf
    unsigned leafStart = (unsigned)primsReordered_.size();
    for (unsigned i = begin; i < end; ++i)
    {
        primsReordered_.push_back(prims_[buildInfo_[i].primIdx]);
    }

    BuildNode n = {b, leafStart, 0, 0, 0, (unsigned)(end - begin)};
    buildNodes.push_back(n);
    return (unsigned)buildNodes.size() - 1;
}


unsigned BVHAccelerator::BuildHierarchy(unsigned begin, unsigned end, std::vector<BuildNode>& buildNodes)
{
    // calculate Node BBox and centroids Bounds to find maximum spatial extent
    BBox b = buildInfo_[begin].box;
    BBox centroidBounds(b.GetCenter());

    std::for_each(&buildInfo_[0] + begin, &buildInfo_[0] + end, [&b, &centroidBounds, this](BuildInfo const& info)
    {
        b = BBoxUnion(b, info.box);
        centroidBounds = BBoxUnion(centroidBounds, info.box.GetCenter());
    });

    // axis to sort along
    float sahValue;
    int primCount = end - begin;
    int axis = centroidBounds.GetMaxDim();
    int mid = 0;
    
    std::sort(&buildInfo_[0] + begin, &buildInfo_[0] + end, [axis](BuildInfo const& i1, BuildInfo const& i2)
              {
                  return i1.box.GetCenter()[axis] < i2.box.GetCenter()[axis];
              });

    if (centroidBounds.GetMinPoint()[axis] == centroidBounds.GetMaxPoint()[axis])
    {
        if (primCount <= maxNodePrims_)
        {
            return CreateLeafNode(buildNodes, begin, end, b);
        }
        else
        {
            mid = (begin + end)/2;
        }
    }
    else
    {
        mid = FindBestSplit(begin, end, b, centroidBounds, sahValue);
        assert(mid >= begin && mid <= end);
    }

    // Decide whether to split or to create new leaf
    if (primCount > maxNodePrims_ || (sahValue < primCount && primCount > 1))
    {
        BuildNode n = {b, static_cast<unsigned int>(axis), 0, 0, 0};
        
        n.left  = BuildHierarchy(begin, mid, buildNodes);
        n.right = BuildHierarchy(mid, end, buildNodes);

        buildNodes[n.left].parent  = (unsigned)buildNodes.size();
        buildNodes[n.right].parent = (unsigned)buildNodes.size();

        buildNodes.push_back(n);

        return (unsigned)buildNodes.size() - 1;
    }
    else
    {
        return CreateLeafNode(buildNodes, begin, end, b);
    }
}

unsigned   BVHAccelerator::FindBestSplit(unsigned begin, unsigned end, BBox const& box, BBox const& centroidBounds, float& sahValue)
{
    // SAH implementation
    // calc centroids histogram
    unsigned const kNumBins = 10;

    if (end - begin < kNumBins)
    {
        return (begin + end)/2;
    }

    struct bin
    {
        BBox box;
        unsigned count;
    };

    bin bins[kNumBins];

    int dim = centroidBounds.GetMaxDim();

    float centroidRng = centroidBounds.GetExtents()[dim];
    float binRng = centroidRng / kNumBins;

    for (unsigned i = 0; i < kNumBins; ++i)
    {
        vector3 d = vector3(0,0,0);
        d[dim] = i * binRng;
        bins[i].count = 0;
        bins[i].box = BBox(centroidBounds.GetMinPoint() + d);
    }

    for (unsigned i = begin; i < end; ++i)
    {
        BBox b = buildInfo_[i].box;
        unsigned bin_idx = (unsigned)std::min<float>(kNumBins * ((b.GetCenter()[dim] - centroidBounds.GetMinPoint()[dim]) / centroidRng), kNumBins-1);

        assert(bin_idx >= 0);

        ++bins[bin_idx].count;
        bins[bin_idx].box = BBoxUnion(bins[bin_idx].box, b);
    }

    float sahCost[kNumBins-1];

    for (unsigned i = 0; i < kNumBins - 1; ++i)
    {
        BBox h1Box = bins[0].box;
        unsigned h1Count = bins[0].count;

        for(unsigned j = 1; j <= i; ++j)
        {
            h1Box = BBoxUnion(h1Box, bins[j].box);
            h1Count += bins[j].count;
        }

        BBox h2Box = bins[i + 1].box;
        unsigned h2Count = bins[i + 1].count;

        for(unsigned j = i + 2; j < kNumBins; ++j)
        {
            h2Box = BBoxUnion(h2Box, bins[j].box);
            h2Count += bins[j].count;
        }

        sahCost[i] = 1.0f + (h1Count * h1Box.GetSurfaceArea() + h2Count * h2Box.GetSurfaceArea())/box.GetSurfaceArea();
    }

    float minSahCost = sahCost[0];
    unsigned minSahSplitIdx = 0;

    for (unsigned i = 1; i < kNumBins - 1; ++i)
    {
        if (sahCost[i] < minSahCost)
        {
            minSahCost = sahCost[i];
            minSahSplitIdx = i;
        }
    }

    float border = bins[minSahSplitIdx + 1].box.GetCenter()[dim];
    auto iter = std::partition(&buildInfo_[0] + begin, &buildInfo_[0] + end, [=](BuildInfo const& info)
    {
        return info.box.GetCenter()[dim] < border;
    });

    sahValue = minSahCost;

    return (unsigned)((iter - &buildInfo_[begin]) + begin);
}

BVHAccelerator::Triangle const* BVHAccelerator::GetPrimitives() const
{
    return &primsReordered_[0];
}

unsigned int    BVHAccelerator::GetPrimitiveCount() const
{
    return (unsigned int)primsReordered_.size();
}

BVHAccelerator::Node const*  BVHAccelerator::GetNodes() const
{
    return &nodes_[0];
}

unsigned int BVHAccelerator::GetNodeCount() const
{
    return (unsigned int)nodes_.size();
}

int BVHAccelerator::GetRootNode() const
{
    return root_;
}

unsigned BVHAccelerator::Linearize(unsigned buildNodeIdx, unsigned parent, std::vector<BuildNode> const& buildNodes)
{
    /// put the Node into nodes_ list
    Node n;
    n.box = buildNodes[buildNodeIdx].box;
    n.primStartIdx = buildNodes[buildNodeIdx].primStartIdx;
    n.primCount = buildNodes[buildNodeIdx].primCount;
    n.parent = parent;

    /// keep current index
    unsigned index = (unsigned)nodes_.size();
    nodes_.push_back(n);

    /// if this is internal Node - linearize children nodes
    if (n.primCount == 0)
    {
        Linearize(buildNodes[buildNodeIdx].left, index, buildNodes);

        nodes_[index].right = Linearize(buildNodes[buildNodeIdx].right, index, buildNodes);
    }

    return index;
}

void BVHAccelerator::Traverse(unsigned buildNodeIdx, std::vector<BuildNode> const& buildNodes, BVH::NodeId id, BVH::ChildRel rel)
{
    if (buildNodes[buildNodeIdx].primCount == 0)
    {
        BVH::SplitAxis axis = buildNodes[buildNodeIdx].primStartIdx == 0 ? BVH::SplitAxis::SA_X :
                             (buildNodes[buildNodeIdx].primStartIdx == 1 ? BVH::SplitAxis::SA_Y :
                                                                           BVH::SplitAxis::SA_Z);
        
        
        id = bvh_.CreateInternalNode(id, rel, axis, buildNodes[buildNodeIdx].box);
        Traverse(buildNodes[buildNodeIdx].left, buildNodes, id, BVH::CR_LEFT);
        Traverse(buildNodes[buildNodeIdx].right, buildNodes, id, BVH::CR_RIGHT);
    }
    else
    {
        std::vector<unsigned> indices;
        for (int i = 0; i < buildNodes[buildNodeIdx].primCount; ++i)
            indices.push_back(buildNodes[buildNodeIdx].primStartIdx + i);
        bvh_.CreateLeafNode(id, rel, buildNodes[buildNodeIdx].box, &indices[0], buildNodes[buildNodeIdx].primCount);
    }
}
