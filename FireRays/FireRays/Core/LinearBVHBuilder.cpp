//
//  LinearBVHBuilder.cpp
//  FireRays
//
//  Created by dmitryk on 24.03.14.
//
//

#include "LinearBVHBuilder.h"

#include "utils.h"

#include <algorithm>


static int clz (int x)
{
    if (x ==0)
        return 32;
    int n=0;
    if ((x & 0xFFFF0000) == 0) { n += 16; x =x << 16;} //1111 1111 1111 1111 0000 0000 0000 0000 // 16 bits from left are zero! so we omit 16left bits
    if ((x & 0xFF000000) == 0){ n = n +  8; x = x <<  8;} // 8 left bits are 0
    if ((x & 0xF0000000) ==0){ n = n +  4; x = x <<  4;} // 4 left bits are 0
    if ((x & 0xC0000000) == 0){ n =n +  2, x = x <<  2;}  // 110000....0 2 left bits are zero
    if ((x & 0x80000000) == 0){n = n +  1, x = x <<  1;} // first left bit is zero
    return n;
}

static cl_float3 calc_centroid(cl_float3 v1, cl_float3 v2, cl_float3 v3)
{
    cl_float3 res;
    res.s[0] = (v1.s[0] + v2.s[0] + v3.s[0]) * (1.f/3.f);
    res.s[1] = (v1.s[1] + v2.s[1] + v3.s[1]) * (1.f/3.f);
    res.s[2] = (v1.s[2] + v2.s[2] + v3.s[2]) * (1.f/3.f);
    return res;
}

static vector3opt cl_float3_to_vector3(cl_float3 v)
{
    return vector3opt(v.s[0], v.s[1], v.s[2]);
}

static cl_float3 vector3_to_cl_float3(vector3opt const& v)
{
    cl_float3 res;
    res.s[0] = v.x;
    res.s[1] = v.y;
    res.s[2] = v.z;
    
    return res;
}

void LinearBVHBuilder::CalcCentroidsAndBounds()
{
    bbox_ = BBox();
    
    centroids_.reserve(prims_.size());
    std::for_each(prims_.begin(), prims_.end(), [this](Primitive const& prim)
                  {
                      cl_float3 c = calc_centroid(positions_[prim.i1], positions_[prim.i2], positions_[prim.i3]);
                      centroids_.push_back(c);
                      
                      bbox_ = BBoxUnion(bbox_, cl_float3_to_vector3(positions_[prim.i1]));
                      bbox_ = BBoxUnion(bbox_, cl_float3_to_vector3(positions_[prim.i2]));
                      bbox_ = BBoxUnion(bbox_, cl_float3_to_vector3(positions_[prim.i3]));
                  }
                  );
}

void LinearBVHBuilder::Build()
{
    CalcCentroidsAndBounds();
    
    CalcMortonCodes();
    
    SortMortonCodes();
    
    EmitBVH();
    
    BuildNodeFromGPU(GetBVH().GetPreRootNode(), BVH::ChildRel::CR_LEFT, 0);
    
    //BuildNode(GetBVH().GetPreRootNode(), BVH::ChildRel::CR_LEFT, 0, prims_.size() - 1, 0);
}

void LinearBVHBuilder::CalcMortonCodes()
{
//    CPU code
//    const int kGridResolution = 1024;
//    
//    mortonCodes_.resize(centroids_.size());
//    
//    vector3 extents = bbox_.GetExtents();
//    vector3 minPoint = bbox_.GetMinPoint();
//    vector3 rangeStep = vector3(kGridResolution / extents.x(), kGridResolution / extents.y(), kGridResolution / extents.z());
//    
//    for (int i = 0; i < centroids_.size(); ++i)
//    {
//        vector3 c = cl_float3_to_vector3(centroids_[i]);
//        vector3 p = clamp((c - minPoint) * rangeStep, vector3(0,0,0), vector3(kGridResolution-1, kGridResolution-1, kGridResolution-1));
//        
//        mortonCodes_[i].first = CalcMortonCode((unsigned)p.x(), (unsigned)p.y(), (unsigned)p.z());
//        mortonCodes_[i].second = i;
//    }
    
    int numPrimitives = prims_.size();
    int blockSize = 64;
    int numBlocks = (numPrimitives + blockSize - 1) / blockSize;
    
    CLWKernel calcMortonCodesKernel = program_.GetKernel("assign_morton_codes");
    
    cl_float3 min =  vector3_to_cl_float3(bbox_.min);
    cl_float3 ext =  vector3_to_cl_float3(bbox_.GetExtents());
    
    calcMortonCodesKernel.SetArg(0, min);
    calcMortonCodesKernel.SetArg(1, ext);
    calcMortonCodesKernel.SetArg(2, gpu_positions_);
    calcMortonCodesKernel.SetArg(3, gpu_prims_);
    calcMortonCodesKernel.SetArg(4, (cl_uint)numPrimitives);
    calcMortonCodesKernel.SetArg(5, gpu_morton_codes_);
    calcMortonCodesKernel.SetArg(6, gpu_indices_);
    
    context_.Launch1D(0, numBlocks * blockSize, blockSize, calcMortonCodesKernel);
}

int LinearBVHBuilder::CalcMortonCode(int xi, int yi, int zi)
{
    int result = 0;
    
    for (int i = 9; i >= 0; --i)
    {
        char tmp = (((xi >> i) & 1) << 2) | (((yi >> i) & 1) << 1) | ((zi >> i) & 1);
        result |= (tmp << (i * 3));
    }
    
    return result;
}

void LinearBVHBuilder::SortMortonCodes()
{
    int numPrimitives = prims_.size();
    
    codes_.resize(numPrimitives);
    std::vector<int> indices(numPrimitives);
    
    pp_.SortRadix(0, gpu_morton_codes_, gpu_morton_codes_sorted_, gpu_indices_, gpu_indices_sorted_);
    
    context_.ReadBuffer(0, gpu_indices_sorted_, &indices[0], indices.size()).Wait();
    context_.ReadBuffer(0, gpu_morton_codes_sorted_, &codes_[0], codes_.size()).Wait();
    

    int blockSize = 64;
    int numBlocks = (numPrimitives + blockSize - 1) / blockSize;
    
    CLWKernel reorderKernel = program_.GetKernel("reorder_primitives");
    reorderKernel.SetArg(0, gpu_indices_sorted_);
    reorderKernel.SetArg(1, gpu_prims_);
    reorderKernel.SetArg(2, (cl_uint)numPrimitives);
    reorderKernel.SetArg(3, gpu_prims_sorted_);
    
    context_.Launch1D(0, numBlocks * blockSize, blockSize, reorderKernel);

    context_.ReadBuffer(0, gpu_prims_sorted_, (cl_uint4*)&prims_[0], numPrimitives).Wait();
}

void LinearBVHBuilder::EmitBVH()
{
    int numPrimitives = prims_.size();
    int kBlockSize = 64;
    
    CLWKernel processLevelKernel = program_.GetKernel("process_split_requests");
    CLWKernel writeNodesKernel   = program_.GetKernel("write_nodes");
    CLWKernel fitBoxKernel       = program_.GetKernel("fit_bbox");
    
    // Initialize the queue
    SplitRequest req = {0, numPrimitives-1, -1, 0};
    context_.WriteBuffer(0, gpu_split_request_queue_0_, &req, 1);
    
    cl_int numRequests = 1;
    cl_int numNodes    = 0;
    
    auto fromQueue = &gpu_split_request_queue_0_;
    auto toQueue   = &gpu_split_request_queue_1_;
    
    std::vector<std::pair<cl_int, cl_int> > nodeLevelOffsets;
    
    while (numRequests > 0)
    {
        processLevelKernel.SetArg(0, gpu_morton_codes_sorted_);
        processLevelKernel.SetArg(1, *fromQueue);
        processLevelKernel.SetArg(2, numRequests);
        processLevelKernel.SetArg(3, gpu_split_request_count_);
        
        int numBlocks = (numRequests + kBlockSize - 1) / kBlockSize;
    
        context_.Launch1D(0, numBlocks * kBlockSize, kBlockSize, processLevelKernel);
        
        //std::vector<SplitRequest> temp2(numRequests);
        //context_.ReadBuffer(0, *fromQueue, &temp2[0], numRequests).Wait();
    
        cl_int lastRequest = 0;
        context_.ReadBuffer(0, gpu_split_request_count_, &lastRequest, numRequests - 1, 1).Wait();
        
       // std::vector<cl_int> temp1(numRequests);
        //context_.ReadBuffer(0, gpu_split_request_count, &temp1[0], numRequests).Wait();
        
        pp_.ScanExclusiveAdd(0, gpu_split_request_count_, gpu_split_request_count_).Wait();
    
        cl_int count = 0;
        context_.ReadBuffer(0, gpu_split_request_count_, &count, numRequests - 1, 1).Wait();

        writeNodesKernel.SetArg(0, *fromQueue);
        writeNodesKernel.SetArg(1, numRequests);
        writeNodesKernel.SetArg(2, gpu_split_request_count_);
        writeNodesKernel.SetArg(3, *toQueue);
        writeNodesKernel.SetArg(4, gpu_nodes_);
        writeNodesKernel.SetArg(5, numNodes);
        
        context_.Launch1D(0, numBlocks * kBlockSize, kBlockSize, writeNodesKernel);
    
        nodeLevelOffsets.push_back(std::make_pair(numNodes, numRequests));
        
        numNodes += numRequests;
        numRequests = count + lastRequest;
    
        
        assert(numNodes < numPrimitives * 10);
        
        std::swap(fromQueue, toQueue);
    }
    
    for(int i = nodeLevelOffsets.size() - 1; i >= 0; --i)
    {
        cl_int numLevelNodes = nodeLevelOffsets[i].second;
        cl_int levelOffset = nodeLevelOffsets[i].first;
        
        fitBoxKernel.SetArg(0, gpu_positions_);
        fitBoxKernel.SetArg(1, gpu_prims_sorted_);
        fitBoxKernel.SetArg(2, gpu_nodes_);
        fitBoxKernel.SetArg(3, numLevelNodes);
        fitBoxKernel.SetArg(4, levelOffset);
        
        int numBlocks = (numLevelNodes + kBlockSize - 1) / kBlockSize;
        
        context_.Launch1D(0, numBlocks * kBlockSize, kBlockSize, fitBoxKernel);
    }
    
    nodes_.resize(numNodes);
    context_.ReadBuffer(0, gpu_nodes_, &nodes_[0], numNodes).Wait();
}

BVH::NodeId LinearBVHBuilder::BuildNode(BVH::NodeId parentNode, BVH::ChildRel rel, int first, int last, unsigned level)
{
    assert(first <= last);
    
    BBox bbox = BBox();
    for (int i = first; i <= last; ++i)
    {
        Primitive prim = prims_[i];
        bbox = BBoxUnion(bbox, cl_float3_to_vector3(positions_[prim.i1]));
        bbox = BBoxUnion(bbox, cl_float3_to_vector3(positions_[prim.i2]));
        bbox = BBoxUnion(bbox, cl_float3_to_vector3(positions_[prim.i3]));

    }
    
    if (first == last)
    {
        std::vector<unsigned> primIndices;
        primIndices.push_back(first);
        return GetBVH().CreateLeafNode(parentNode, rel, bbox, &primIndices[0], primIndices.size());
    }
    else
    {
        int split = FindSplit(first, last);
        
        BVH::NodeId id = GetBVH().CreateInternalNode(parentNode, rel, static_cast<BVH::SplitAxis>(0), bbox);
        
        BuildNode(id, BVH::ChildRel::CR_LEFT,  first, split, ++level);
        BuildNode(id, BVH::ChildRel::CR_RIGHT, split + 1, last, ++level);
    }
}

int LinearBVHBuilder::FindSplit(int first, int last)
{
    int firstCode = codes_[first];
    int lastCode = codes_[last];
    
    if (firstCode == lastCode)
        return (first + last) >> 1;
    
    int commonPrefix = clz(firstCode ^ lastCode);
    
    // Use binary search to find where the next bit differs.
    // Specifically, we are looking for the highest object that
    // shares more than commonPrefix bits with the first one.
    
    int split = first; // initial guess
    int step = last - first;
    
    do
    {
        step = (step + 1) >> 1; // exponential decrease
        int newSplit = split + step; // proposed new position
        
        if (newSplit < last)
        {
            unsigned int splitCode = codes_[newSplit];
            int splitPrefix = clz(firstCode ^ splitCode);
            if (splitPrefix > commonPrefix)
                split = newSplit; // accept proposal
        }
    }
    while (step > 1);
    
    return split;
    
}

unsigned         LinearBVHBuilder::GetPrimitiveCount() const
{
    return prims_.size();
}

BVHBuilderBase::Primitive const* LinearBVHBuilder::GetPrimitives() const
{
    return &prims_[0];
}

void LinearBVHBuilder::InitOpenCL()
{
    // Create platform
    std::vector<CLWPlatform> platforms;
    CLWPlatform::CreateAllPlatforms(platforms);
    auto platform = platforms[1];
    platforms.clear();
    
    // Create OpenCL context
    context_ = CLWContext::Create(platform.GetDevice(1));
    
    // Load source code
    std::vector<char> sourceCode;
    LoadFileContents("../../../FireRays/CL/lbvh.cl", sourceCode);
    
    // Create program
    program_ = CLWProgram::CreateFromSource(sourceCode, context_);
    
    // Create GPU buffers
    size_t numVertices = positions_.size();
    size_t numPrimitives = prims_.size();
    
    gpu_positions_              = context_.CreateBuffer<cl_float3>(numVertices, CL_MEM_READ_WRITE);
    gpu_prims_                  = context_.CreateBuffer<cl_uint4>(numPrimitives, CL_MEM_READ_WRITE);
    gpu_morton_codes_           = context_.CreateBuffer<cl_int>(numPrimitives, CL_MEM_READ_WRITE);
    gpu_indices_                = context_.CreateBuffer<cl_int>(numPrimitives, CL_MEM_READ_WRITE);
    gpu_morton_codes_sorted_    = context_.CreateBuffer<cl_int>(numPrimitives, CL_MEM_READ_WRITE);
    gpu_indices_sorted_         = context_.CreateBuffer<cl_int>(numPrimitives, CL_MEM_READ_WRITE);
    gpu_prims_sorted_           = context_.CreateBuffer<cl_uint4>(numPrimitives, CL_MEM_READ_WRITE);
    gpu_split_request_queue_0_   = context_.CreateBuffer<SplitRequest>(numPrimitives * 3, CL_MEM_READ_WRITE);
    gpu_split_request_queue_1_   = context_.CreateBuffer<SplitRequest>(numPrimitives * 3, CL_MEM_READ_WRITE);
    gpu_split_request_count_     = context_.CreateBuffer<cl_int>(numPrimitives * 3, CL_MEM_READ_WRITE);
    gpu_nodes_                   = context_.CreateBuffer<Node>(numPrimitives * 10, CL_MEM_READ_WRITE);
    
    // Write positions and primitive indices to GPU buffers
    context_.WriteBuffer(0, gpu_positions_, (cl_float3*)&positions_[0], numVertices  ).Wait();
    context_.WriteBuffer(0, gpu_prims_,   (cl_uint4*)&prims_[0],      numPrimitives).Wait();
    
    // Create parallel primitives object
    pp_ = CLWParallelPrimitives(context_);
}

BVH::NodeId LinearBVHBuilder::BuildNodeFromGPU(BVH::NodeId parentNode, BVH::ChildRel rel, int idx)
{
    Node node = nodes_[idx];
    
    BBox bbox = BBox();
    for (int i = node.first; i <= node.last;++i)
    {
        Primitive prim = prims_[i];
        bbox = BBoxUnion(bbox, cl_float3_to_vector3(positions_[prim.i1]));
        bbox = BBoxUnion(bbox, cl_float3_to_vector3(positions_[prim.i2]));
        bbox = BBoxUnion(bbox, cl_float3_to_vector3(positions_[prim.i3]));
    }
    
    if (node.first == node.last)
    {
        std::vector<unsigned> primIndices;
        primIndices.push_back(node.first);
        return GetBVH().CreateLeafNode(parentNode, rel, bbox, &primIndices[0], primIndices.size());
    }
    else
    {
        BVH::NodeId id = GetBVH().CreateInternalNode(parentNode, rel, static_cast<BVH::SplitAxis>(0), bbox);
        
        BuildNodeFromGPU(id, BVH::ChildRel::CR_LEFT,  node.left);
        BuildNodeFromGPU(id, BVH::ChildRel::CR_RIGHT, node.right);
    }
}





