//
//  LinearBVHBuilder.h
//  FireRays
//
//  Created by dmitryk on 24.03.14.
//
//

#ifndef LINEARBVHBUILDER_H
#define LINEARBVHBUILDER_H

#include <iostream>

#include "BVH.h"
#include "BVHBuilderBase.h"
#include "CLW.h"

class LinearBVHBuilder : public BVHBuilderBase
{
public:
    // Construct BVH from an indexed vertex list
    template <typename T> LinearBVHBuilder(T const* vertices, unsigned int vertexCount, unsigned const* indices, unsigned indexCount, unsigned const* materials);
    
    // Start building process
    void Build();
    
    unsigned         GetPrimitiveCount() const;
    Primitive const* GetPrimitives() const;
    
protected:
    void CalcCentroidsAndBounds();
    void CalcMortonCodes();
    void SortMortonCodes();
    void EmitBVH();
    
    int CalcMortonCode(int xi, int yi, int zi);
    
    BVH::NodeId BuildNode(BVH::NodeId parentNode, BVH::ChildRel rel, int first, int last, unsigned level);
    int  FindSplit(int first, int last);
    void InitOpenCL();
    
    BVH::NodeId BuildNodeFromGPU(BVH::NodeId parentNode, BVH::ChildRel rel, int idx);
    
    

    
private:
    
    struct Box
    {
        cl_float3 min;
        cl_float3 max;
    };
    
    struct SplitRequest
    {
        cl_int first;
        cl_int last;
        
        cl_int parent;
        cl_int split;
    };
    
    struct Node
    {
        Box bbox;
        int parent;
        int left;
        int right;
        int first;
        int last;
    };
    
    // Data members
    std::vector<Primitive>    prims_;
    std::vector<cl_float3>    positions_;
    std::vector<cl_float3>    centroids_;
    std::vector<std::pair<int, int> >   mortonCodes_;
    std::vector<int> codes_;
    
    BBox bbox_;
    std::vector<Node> nodes_;
    
    // CL stuff
    CLWContext context_;
    CLWProgram program_;
    CLWParallelPrimitives pp_;
    
    CLWBuffer<cl_float3> gpu_positions_;
    CLWBuffer<cl_uint4>  gpu_prims_;
    CLWBuffer<cl_uint4>  gpu_prims_sorted_;
    
    CLWBuffer<cl_int>    gpu_morton_codes_;
    CLWBuffer<cl_int>    gpu_indices_;
    
    CLWBuffer<cl_int>    gpu_morton_codes_sorted_;
    CLWBuffer<cl_int>    gpu_indices_sorted_;
    
    CLWBuffer<SplitRequest> gpu_split_request_queue_0_;
    CLWBuffer<SplitRequest> gpu_split_request_queue_1_;
    
    CLWBuffer<cl_int>       gpu_split_request_count_;
    CLWBuffer<Node>         gpu_nodes_;
};

template <typename T> LinearBVHBuilder::LinearBVHBuilder(T const* vertices, unsigned int vertexCount, unsigned const* indices, unsigned indexCount, unsigned const* materials)
{
    positions_.resize(vertexCount);
    
    /// copy vertex data
    for (int i = 0; i < vertexCount; ++i)
    {
        positions_[i].s[0] = vertices[i].position.x();
        positions_[i].s[1] = vertices[i].position.y();
        positions_[i].s[2] = vertices[i].position.z();
    }
    
    /// build primitives list
    prims_.resize(indexCount/3);
    for (unsigned i = 0; i < indexCount; i += 3)
    {
        BBox b(vertices[indices[i]].position, vertices[indices[i+1]].position);
        b = BBoxUnion(b, vertices[indices[i+2]].position);
        
        Primitive t = {indices[i], indices[i + 1], indices[i + 2], materials[i/3]};
        prims_[i/3] = t;
    }
    
    InitOpenCL();
}

#endif //LINEARBVHBUILDER_H

