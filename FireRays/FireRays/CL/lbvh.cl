#define GRID_RESOLUTION 1024

typedef struct
{
    int first;
    int last;
    int parent;
    int split;
} SplitRequest;

typedef struct
{
    float3 min;
    float3 max;
} BBox;

typedef struct
{
    BBox bbox;
    int  parent;
    int  left;
    int  right;
    int  first;
    int  last;
} Node;

//float3 make_float3(float x, float y, float z)
//{
//    float3 res;
//    res.x = x;
//    res.y = y;
//    res.z = z;
//    return res;
//}
//
//int3 make_int3(int x, int y, int z)
//{
//    int3 res;
//    res.x = x;
//    res.y = y;
//    res.z = z;
//    return res;
//}

//
// the following two functions are from https://devblogs.nvidia.com/parallelforall/thinking-parallel-part-iii-tree-construction-gpu/
int expand_bits(int v)
{
    v = (v * 0x00010001u) & 0xFF0000FFu;
    v = (v * 0x00000101u) & 0x0F00F00Fu;
    v = (v * 0x00000011u) & 0xC30C30C3u;
    v = (v * 0x00000005u) & 0x49249249u;
    return v;
}

int calc_morton_code(int3 p)
{
    int xx = expand_bits(p.x);
    int yy = expand_bits(p.y);
    int zz = expand_bits(p.z);
    return xx * 4 + yy * 2 + zz;
}

int3 calc_point_bin(float3 min, float3 extents, float3 p)
{
    float3 range_step = make_float3(GRID_RESOLUTION / extents.x, GRID_RESOLUTION / extents.y, GRID_RESOLUTION / extents.z);
    float3 res = clamp((p - min) * range_step, 0.f, (float)(GRID_RESOLUTION - 1));
    return make_int3((int)res.x, (int)res.y, (int)res.z);
}

__kernel void assign_morton_codes(         float3  in_bbox_min,
                                           float3  in_bbox_extents,
                                  __global float3* in_positions,
                                  __global uint4*  in_prims,
                                           uint    in_num_primitives,
                                  __global int*    out_morton_codes,
                                  __global int*    out_prim_indices)
{
    int global_id = get_global_id(0);
    
    if (global_id < in_num_primitives)
    {
        // fetch indices
        uint4  prim = in_prims[global_id];
        float3 v1   = in_positions[prim.x];
        float3 v2   = in_positions[prim.y];
        float3 v3   = in_positions[prim.z];
        
        float3 c = (v1 + v2 + v3) * (1.f/3.f);
        
        out_morton_codes[global_id] = calc_morton_code(calc_point_bin(in_bbox_min, in_bbox_extents, c));
        out_prim_indices[global_id] = global_id;
    }
}


__kernel void reorder_primitives(
                                 __global int*    in_prim_indices,
                                 __global uint4*  in_prims,
                                          uint    in_num_primitives,
                                 __global uint4*  out_prims
                                 )
{
    int global_id = get_global_id(0);
    
    if (global_id < in_num_primitives)
    {
        uint4  prim = in_prims[in_prim_indices[global_id]];
        out_prims[global_id] = prim;
    }
}

int find_split(__global int* sortedMortonCodes,
                        int  first,
                        int  last)
{
    int firstCode = sortedMortonCodes[first];
    int lastCode =  sortedMortonCodes[last];
    
    if (firstCode == lastCode)
        return (first + last) >> 1;
    
    // Calculate the number of highest bits that are the same
    // for all objects, using the count-leading-zeros intrinsic.
    
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
            unsigned int splitCode = sortedMortonCodes[newSplit];
            int splitPrefix = clz(firstCode ^ splitCode);
            if (splitPrefix > commonPrefix)
                split = newSplit; // accept proposal
        }
    }
    while (step > 1);
    
    return split;
}


__kernel void process_split_requests( __global int*          in_morton_codes,
                                      __global SplitRequest* inout_split_request_queue,
                                               uint          in_num_requests,
                                      __global int*          out_split_request_count)

{
    int global_id = get_global_id(0);
    
    if (global_id < in_num_requests)
    {
        __global SplitRequest* req = inout_split_request_queue + global_id;
        int first = req->first;
        int last  = req->last;
        
        
        if (last == first)
        {
            req->split = -1;
            out_split_request_count[global_id] = 0;
        }
        else
        {
            int split = find_split(in_morton_codes, first, last);
            
            if (split == first || split == last)
            {
                split = (first + last) >> 1;
            }

            req->split = split;
            out_split_request_count[global_id] = 2;
        }
    }
}

__kernel void write_nodes(
                          __global SplitRequest* in_split_request_queue,
                                   uint          in_num_requests,
                          __global int*          in_child_offsets,
                          __global SplitRequest* out_split_request_queue,
                          __global Node*         out_nodes,
                                   uint          level_offset
                          )
{
    int global_id = get_global_id(0);
    
    if (global_id < in_num_requests)
    {
        __global SplitRequest* req = in_split_request_queue + global_id;
        
        int child_offset = in_child_offsets[global_id];
        int next_level_offset = level_offset + in_num_requests;
        
        Node node;
        node.parent = req->parent;
        if (req->split != -1)
        {
            node.left = next_level_offset + child_offset;
            node.right = next_level_offset + child_offset + 1;
            node.first = req->first;
            node.last = req->last;
            
            SplitRequest newRequest;
            
            newRequest.split = 0;
            newRequest.parent = level_offset + global_id;
            
            newRequest.first = req->first;
            newRequest.last  = req->split;
            
            out_split_request_queue[child_offset] = newRequest;
            
            newRequest.first = req->split + 1;
            newRequest.last  = req->last;
            
            out_split_request_queue[child_offset + 1] = newRequest;
        }
        else
        {
            node.left = node.right = -1;
            node.first = req->first;
            node.last = req->last;
        }
        
        out_nodes[level_offset + global_id] = node;
    }
}

BBox create_bbox(float3 v1, float3 v2, float3 v3)
{
    BBox res;
    res.min = fmin(fmin(v1, v2), v3);
    res.max = fmax(fmax(v1, v2), v3);
    return res;
}

BBox union_bbox(BBox b1, BBox b2)
{
    BBox res;
    res.min = fmin(b1.min, b2.min);
    res.max = fmax(b1.max, b2.max);
    return res;
}

__kernel void fit_bbox(
                       __global float3* in_positions,
                       __global uint4*  in_prims,
                       __global Node*   inout_nodes,
                                uint    in_num_nodes,
                                uint    level_offset
                       )
{
    int global_id = get_global_id(0);
    
    if (global_id < in_num_nodes)
    {
        __global Node* node_ptr = inout_nodes + level_offset + global_id;
        
        if (node_ptr->left != -1)
        {
            Node left  = inout_nodes[node_ptr->left];
            Node right = inout_nodes[node_ptr->right];
            node_ptr->bbox = union_bbox(left.bbox, right.bbox);
        }
        else
        {
            uint4 prim = in_prims[node_ptr->first];
            node_ptr->bbox = create_bbox(in_positions[prim.x], in_positions[prim.y], in_positions[prim.z]);
        }
    }
}