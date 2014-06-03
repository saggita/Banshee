//
//  CLWContext.h
//  CLW
//
//  Created by dmitryk on 01.12.13.
//  Copyright (c) 2013 dmitryk. All rights reserved.
//


// --------------------- HELPERS ------------------------
//#define INT_MAX 0x7FFFFFFF

// -------------------- MACRO --------------------------
// Apple OCL compiler has this by default, 
// so embrace with #ifdef in the future
#define DEFINE_MAKE_4(type)\
type##4 make_##type##4(type x, type y, type z, type w)\
{\
    type##4 res;\
    res.x = x;\
    res.y = y;\
    res.z = z;\
    res.w = w;\
    return res;\
}

// Multitype macros to handle parallel primitives
#define DEFINE_SAFE_LOAD_4(type)\
type##4 safe_load_##type##4(__global type##4* source, uint idx, uint sizeInTypeUnits)\
{\
    type##4 res = make_##type##4(0, 0, 0, 0);\
    if (((idx + 1) << 2)  <= sizeInTypeUnits)\
        res = source[idx];\
    else\
    {\
        if ((idx << 2) < sizeInTypeUnits) res.x = source[idx].x;\
        if ((idx << 2) + 1 < sizeInTypeUnits) res.y = source[idx].y;\
        if ((idx << 2) + 2 < sizeInTypeUnits) res.z = source[idx].z;\
    }\
    return res;\
}

#define DEFINE_SAFE_STORE_4(type)\
void safe_store_##type##4(type##4 val, __global type##4* dest, uint idx, uint sizeInTypeUnits)\
{\
    if ((idx + 1) * 4  <= sizeInTypeUnits)\
        dest[idx] = val;\
    else\
    {\
        if (idx*4 < sizeInTypeUnits) dest[idx].x = val.x;\
        if (idx*4 + 1 < sizeInTypeUnits) dest[idx].y = val.y;\
        if (idx*4 + 2 < sizeInTypeUnits) dest[idx].z = val.z;\
    }\
}

#define DEFINE_GROUP_SCAN_EXCLUSIVE(type)\
void group_scan_exclusive_##type(int localId, int groupSize, __local type* shmem)\
{\
    for (int stride = 1; stride <= (groupSize >> 1); stride <<= 1)\
    {\
        if (localId < groupSize/(2*stride))\
        {\
            shmem[2*(localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1] + shmem[(2*localId + 1)*stride-1];\
        }\
        barrier(CLK_LOCAL_MEM_FENCE);\
    }\
    if (localId == 0)\
        shmem[groupSize - 1] = 0;\
    barrier(CLK_LOCAL_MEM_FENCE);\
    for (int stride = (groupSize >> 1); stride > 0; stride >>= 1)\
    {\
        if (localId < groupSize/(2*stride))\
        {\
            type temp = shmem[(2*localId + 1)*stride-1];\
            shmem[(2*localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1];\
            shmem[2*(localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1] + temp;\
        }\
        barrier(CLK_LOCAL_MEM_FENCE);\
    }\
}

#define DEFINE_GROUP_SCAN_EXCLUSIVE_PART(type)\
type group_scan_exclusive_part_##type( int localId, int groupSize, __local type* shmem)\
{\
    type sum = 0;\
    for (int stride = 1; stride <= (groupSize >> 1); stride <<= 1)\
    {\
        if (localId < groupSize/(2*stride))\
        {\
            shmem[2*(localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1] + shmem[(2*localId + 1)*stride-1];\
        }\
        barrier(CLK_LOCAL_MEM_FENCE);\
    }\
    if (localId == 0)\
    {\
        sum = shmem[groupSize - 1];\
        shmem[groupSize - 1] = 0;\
    }\
    barrier(CLK_LOCAL_MEM_FENCE);\
    for (int stride = (groupSize >> 1); stride > 0; stride >>= 1)\
    {\
        if (localId < groupSize/(2*stride))\
        {\
            type temp = shmem[(2*localId + 1)*stride-1];\
            shmem[(2*localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1];\
            shmem[2*(localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1] + temp;\
        }\
        barrier(CLK_LOCAL_MEM_FENCE);\
    }\
    return sum;\
}

#define DEFINE_SCAN_EXCLUSIVE(type)\
__kernel void scan_exclusive_##type(__global type const* in_array, __global type* out_array, __local type* shmem)\
{\
    int globalId  = get_global_id(0);\
    int localId   = get_local_id(0);\
    int groupSize = get_local_size(0);\
    int groupId   = get_group_id(0);\
    shmem[localId] = in_array[2*globalId] + in_array[2*globalId + 1];\
    barrier(CLK_LOCAL_MEM_FENCE);\
    group_scan_exclusive_##type(localId, groupSize, shmem);\
    out_array[2 * globalId + 1] = shmem[localId] + in_array[2*globalId];\
    out_array[2 * globalId] = shmem[localId];\
}

#define DEFINE_SCAN_EXCLUSIVE_4(type)\
__kernel void scan_exclusive_##type##4(__global type##4 const* in_array, __global type##4* out_array, uint numElems, __local type* shmem)\
{\
    int globalId  = get_global_id(0);\
    int localId   = get_local_id(0);\
    int groupSize = get_local_size(0);\
    type##4 v1 = safe_load_##type##4(in_array, 2*globalId, numElems);\
    type##4 v2 = safe_load_##type##4(in_array, 2*globalId + 1, numElems);\
    v1.y += v1.x; v1.w += v1.z; v1.w += v1.y;\
    v2.y += v2.x; v2.w += v2.z; v2.w += v2.y;\
    v2.w += v1.w;\
    shmem[localId] = v2.w;\
    barrier(CLK_LOCAL_MEM_FENCE);\
    group_scan_exclusive_##type(localId, groupSize, shmem);\
    v2.w = shmem[localId];\
    int t = v1.w; v1.w = v2.w; v2.w += t;\
    t = v1.y; v1.y = v1.w; v1.w += t;\
    t = v2.y; v2.y = v2.w; v2.w += t;\
    t = v1.x; v1.x = v1.y; v1.y += t;\
    t = v2.x; v2.x = v2.y; v2.y += t;\
    t = v1.z; v1.z = v1.w; v1.w += t;\
    t = v2.z; v2.z = v2.w; v2.w += t;\
    safe_store_##type##4(v2, out_array, 2 * globalId + 1, numElems);\
    safe_store_##type##4(v1, out_array, 2 * globalId, numElems);\
}

#define DEFINE_SCAN_EXCLUSIVE_PART_4(type)\
__kernel void scan_exclusive_part_##type##4(__global type##4 const* in_array, __global type##4* out_array, uint numElems, __global type* out_sums, __local type* shmem)\
{\
    int globalId  = get_global_id(0);\
    int localId   = get_local_id(0);\
    int groupId   = get_group_id(0);\
    int groupSize = get_local_size(0);\
    type##4 v1 = safe_load_##type##4(in_array, 2*globalId, numElems);\
    type##4 v2 = safe_load_##type##4(in_array, 2*globalId + 1, numElems);\
    v1.y += v1.x; v1.w += v1.z; v1.w += v1.y;\
    v2.y += v2.x; v2.w += v2.z; v2.w += v2.y;\
    v2.w += v1.w;\
    shmem[localId] = v2.w;\
    barrier(CLK_LOCAL_MEM_FENCE);\
    type sum = group_scan_exclusive_part_##type(localId, groupSize, shmem);\
    if (localId == 0) out_sums[groupId] = sum;\
    v2.w = shmem[localId];\
    int t = v1.w; v1.w = v2.w; v2.w += t;\
    t = v1.y; v1.y = v1.w; v1.w += t;\
    t = v2.y; v2.y = v2.w; v2.w += t;\
    t = v1.x; v1.x = v1.y; v1.y += t;\
    t = v2.x; v2.x = v2.y; v2.y += t;\
    t = v1.z; v1.z = v1.w; v1.w += t;\
    t = v2.z; v2.z = v2.w; v2.w += t;\
    safe_store_##type##4(v2, out_array, 2 * globalId + 1, numElems);\
    safe_store_##type##4(v1, out_array, 2 * globalId, numElems);\
}

#define DEFINE_GROUP_REDUCE(type)\
void group_reduce_##type(int localId, int groupSize, __local type* shmem)\
{\
    for (int stride = 1; stride <= (groupSize >> 1); stride <<= 1)\
    {\
        if (localId < groupSize/(2*stride))\
        {\
            shmem[2*(localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1] + shmem[(2*localId + 1)*stride-1];\
        }\
        barrier(CLK_LOCAL_MEM_FENCE);\
    }\
}

#define DEFINE_DISTRIBUTE_PART_SUM_4(type)\
__kernel void distribute_part_sum_##type##4( __global type* in_sums, __global type##4* inout_array, uint numElems)\
{\
    int globalId  = get_global_id(0);\
    int groupId   = get_group_id(0);\
    type##4 v1 = safe_load_##type##4(inout_array, 2*globalId, numElems);\
    type##4 v2 = safe_load_##type##4(inout_array, 2*globalId + 1, numElems);\
    type    sum = in_sums[groupId];\
    v1.xyzw += sum;\
    v2.xyzw += sum;\
    safe_store_##type##4(v2, inout_array, 2 * globalId + 1, numElems);\
    safe_store_##type##4(v1, inout_array, 2 * globalId, numElems);\
}


DEFINE_MAKE_4(int)
DEFINE_MAKE_4(float)

DEFINE_SAFE_LOAD_4(int)
DEFINE_SAFE_LOAD_4(float)

DEFINE_SAFE_STORE_4(int)
DEFINE_SAFE_STORE_4(float)

DEFINE_GROUP_SCAN_EXCLUSIVE(int)
DEFINE_GROUP_SCAN_EXCLUSIVE(float)

DEFINE_GROUP_SCAN_EXCLUSIVE_PART(int)
DEFINE_GROUP_SCAN_EXCLUSIVE_PART(float)

DEFINE_SCAN_EXCLUSIVE(int)
DEFINE_SCAN_EXCLUSIVE(float)

DEFINE_SCAN_EXCLUSIVE_4(int)
DEFINE_SCAN_EXCLUSIVE_4(float)

DEFINE_SCAN_EXCLUSIVE_PART_4(int)
DEFINE_SCAN_EXCLUSIVE_PART_4(float)

DEFINE_DISTRIBUTE_PART_SUM_4(int)
DEFINE_DISTRIBUTE_PART_SUM_4(float)

/// Specific function for radix-sort needs
/// Group exclusive add multiscan on 4 arrays of shorts in parallel
/// with 4x reduction in registers
void group_scan_short_4way(int localId, int groupSize,
    short4 mask0,
    short4 mask1,
    short4 mask2,
    short4 mask3,
    __local short* shmem0,
    __local short* shmem1,
    __local short* shmem2,
    __local short* shmem3,
    short4* offset0,
    short4* offset1,
    short4* offset2,
    short4* offset3,
    short4* histogram)
{
    short4 v1 = mask0;
    v1.y += v1.x; v1.w += v1.z; v1.w += v1.y;
    shmem0[localId] = v1.w;
    
    short4 v2 = mask1;
    v2.y += v2.x; v2.w += v2.z; v2.w += v2.y;
    shmem1[localId] = v2.w;
    
    short4 v3 = mask2;
    v3.y += v3.x; v3.w += v3.z; v3.w += v3.y;
    shmem2[localId] = v3.w;
    
    short4 v4 = mask3;
    v4.y += v4.x; v4.w += v4.z; v4.w += v4.y;
    shmem3[localId] = v4.w;
    
    barrier(CLK_LOCAL_MEM_FENCE);
    
    for (int stride = 1; stride <= (groupSize >> 1); stride <<= 1)
    {
        if (localId < groupSize/(2*stride))
        {
            shmem0[2*(localId + 1)*stride-1] = shmem0[2*(localId + 1)*stride-1] + shmem0[(2*localId + 1)*stride-1];
            shmem1[2*(localId + 1)*stride-1] = shmem1[2*(localId + 1)*stride-1] + shmem1[(2*localId + 1)*stride-1];
            shmem2[2*(localId + 1)*stride-1] = shmem2[2*(localId + 1)*stride-1] + shmem2[(2*localId + 1)*stride-1];
            shmem3[2*(localId + 1)*stride-1] = shmem3[2*(localId + 1)*stride-1] + shmem3[(2*localId + 1)*stride-1];
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    
    short4 total;
    total.s0 = shmem0[groupSize - 1];
    total.s1 = shmem1[groupSize - 1];
    total.s2 = shmem2[groupSize - 1];
    total.s3 = shmem3[groupSize - 1];
    
    barrier(CLK_LOCAL_MEM_FENCE);
    
    if (localId == 0)
    {
        shmem0[groupSize - 1] = 0;
        shmem1[groupSize - 1] = 0;
        shmem2[groupSize - 1] = 0;
        shmem3[groupSize - 1] = 0;
    }
    
    barrier(CLK_LOCAL_MEM_FENCE);
    
    for (int stride = (groupSize >> 1); stride > 0; stride >>= 1)
    {
        if (localId < groupSize/(2*stride))
        {
            int temp = shmem0[(2*localId + 1)*stride-1];
            shmem0[(2*localId + 1)*stride-1] = shmem0[2*(localId + 1)*stride-1];
            shmem0[2*(localId + 1)*stride-1] = shmem0[2*(localId + 1)*stride-1] + temp;
            
            temp = shmem1[(2*localId + 1)*stride-1];
            shmem1[(2*localId + 1)*stride-1] = shmem1[2*(localId + 1)*stride-1];
            shmem1[2*(localId + 1)*stride-1] = shmem1[2*(localId + 1)*stride-1] + temp;
            
            temp = shmem2[(2*localId + 1)*stride-1];
            shmem2[(2*localId + 1)*stride-1] = shmem2[2*(localId + 1)*stride-1];
            shmem2[2*(localId + 1)*stride-1] = shmem2[2*(localId + 1)*stride-1] + temp;
            
            temp = shmem3[(2*localId + 1)*stride-1];
            shmem3[(2*localId + 1)*stride-1] = shmem3[2*(localId + 1)*stride-1];
            shmem3[2*(localId + 1)*stride-1] = shmem3[2*(localId + 1)*stride-1] + temp;
        }
        
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    
    v1.w = shmem0[localId];
    
    short t = v1.y; v1.y = v1.w; v1.w += t;
    t = v1.x; v1.x = v1.y; v1.y += t;
    t = v1.z; v1.z = v1.w; v1.w += t;
    *offset0 = v1;
    
    v2.w = shmem1[localId];
    
    t = v2.y; v2.y = v2.w; v2.w += t;
    t = v2.x; v2.x = v2.y; v2.y += t;
    t = v2.z; v2.z = v2.w; v2.w += t;
    *offset1 = v2;
    
    v3.w = shmem2[localId];
    
    t = v3.y; v3.y = v3.w; v3.w += t;
    t = v3.x; v3.x = v3.y; v3.y += t;
    t = v3.z; v3.z = v3.w; v3.w += t;
    *offset2 = v3;
    
    v4.w = shmem3[localId];
    
    t = v4.y; v4.y = v4.w; v4.w += t;
    t = v4.x; v4.x = v4.y; v4.y += t;
    t = v4.z; v4.z = v4.w; v4.w += t;
    *offset3 = v4;

    *histogram = total;
}

// Calculate bool radix mask
short4 radix_mask(int offset, uchar digit, int4 val)
{
    short4 res;
    res.x = ((val.x >> offset) & 3) == digit ? 1 : 0;
    res.y = ((val.y >> offset) & 3) == digit ? 1 : 0;
    res.z = ((val.z >> offset) & 3) == digit ? 1 : 0;
    res.w = ((val.w >> offset) & 3) == digit ? 1 : 0;
    return res;
}

// Choose offset based on radix mask value 
short offset_4way(int val, int offset, short offset0, short offset1, short offset2, short offset3)
{
    switch ((val >> offset) & 3)
    {
        case 0:
            return offset0;
        case 1:
            return offset1;
        case 2:
            return offset2;
        case 3:
            return offset3; 
    }

    return 0;
}

// Perform group split using 2-bits pass
void group_split_radix_2bits(
                            int localId, 
                            int groupSize, 
                            int offset, 
                            int4 val,
                            __local short* shmem, 
                            __global int4* out_local_offsets,
                            short4* histogram)
{
    /// Pointers to radix flag arrays
    __local short* shmem0 = shmem;
    __local short* shmem1 = shmem0 + groupSize;
    __local short* shmem2 = shmem1 + groupSize;
    __local short* shmem3 = shmem2 + groupSize;
    
    /// Radix masks for each digit
    short4 mask0 = radix_mask(offset, 0, val);
    short4 mask1 = radix_mask(offset, 1, val);
    short4 mask2 = radix_mask(offset, 2, val);
    short4 mask3 = radix_mask(offset, 3, val);
    
    /// Resulting offsets
    short4 offset0;
    short4 offset1;
    short4 offset2;
    short4 offset3;

    group_scan_short_4way(localId, groupSize, 
        mask0, mask1, mask2, mask3,
        shmem0, shmem1, shmem2, shmem3,
        &offset0, &offset1, &offset2, &offset3, 
        histogram);

    int4 localOffset;

    localOffset.x = offset_4way(val.x, offset, offset0.x, offset1.x, offset2.x, offset3.x);
    localOffset.y = offset_4way(val.y, offset, offset0.y, offset1.y, offset2.y, offset3.y);
    localOffset.z = offset_4way(val.z, offset, offset0.z, offset1.z, offset2.z, offset3.z);
    localOffset.w = offset_4way(val.w, offset, offset0.w, offset1.w, offset2.w, offset3.w);

    out_local_offsets[localId] = localOffset;
}

int4 safe_load_int4_intmax(__global int4* source, uint idx, uint sizeInInts)
{
    int4 res = make_int4(INT_MAX, INT_MAX, INT_MAX, INT_MAX);
    if (((idx + 1) << 2) <= sizeInInts)
        res = source[idx];
    else
    {
        if ((idx << 2) < sizeInInts) res.x = source[idx].x;
        if ((idx << 2) + 1 < sizeInInts) res.y = source[idx].y;
        if ((idx << 2) + 2 < sizeInInts) res.z = source[idx].z;
    }
    return res;
}

void safe_store_int(int val, __global int* dest, uint idx, uint sizeInInts)
{
    if (idx < sizeInInts) 
        dest[idx] = val;
}

// Split kernel launcher
__kernel void split_4way(int bitshift, __global int4* in_array, uint numElems, __global int* out_histograms, __global int4* out_local_offsets, __local short* shmem)
{
    int globalId  = get_global_id(0);
    int localId   = get_local_id(0);
    int groupSize = get_local_size(0);
    int groupId   = get_group_id(0);
    int numGroups = get_global_size(0) / groupSize;

    /// Load single int4 value
    int4 val = safe_load_int4_intmax(in_array, globalId, numElems);

    short4 localHistogram;
    group_split_radix_2bits(localId, groupSize, bitshift, val, shmem, out_local_offsets + groupId * groupSize,
        &localHistogram);

    if (localId == 0)
    {
        out_histograms[groupId] = localHistogram.x;
        out_histograms[groupId + numGroups] = localHistogram.y;
        out_histograms[groupId + 2 * numGroups] = localHistogram.z;
        out_histograms[groupId + 3 * numGroups] = localHistogram.w;
    }
}

__kernel void scatter_keys(int bitshift,
                           __global int4* in_keys,
                           __global int4* in_values,
                           uint numElems,
                           __global int* in_histograms,
                           __global int4* in_local_offsets,
                           __global int4* out_keys,
                           __global int4* out_values
                           
)
{
    int globalId  = get_global_id(0);
    int localId   = get_local_id(0);
    int groupSize = get_local_size(0);
    int groupId   = get_group_id(0);
    int numGroups = get_global_size(0) / groupSize;
    
    if ((globalId << 2) < numElems)
    {
        int4 key         = safe_load_int4_intmax(in_keys, globalId, numElems);
        int4 value       = safe_load_int4_intmax(in_values, globalId, numElems);
        int4 localOffset = in_local_offsets[globalId];
        
        uchar v = (key.x >> bitshift) & 0x3;
        int scatterAddr = in_histograms[groupId + v * numGroups] + localOffset.x;
        safe_store_int(key.x, (__global int*)out_keys, scatterAddr, numElems);
        safe_store_int(value.x, (__global int*)out_values, scatterAddr, numElems);
        
        v = (key.y >> bitshift) & 0x3;
        scatterAddr = in_histograms[groupId + v * numGroups] + localOffset.y;
        safe_store_int(key.y, (__global int*)out_keys, scatterAddr, numElems);
        safe_store_int(value.y, (__global int*)out_values, scatterAddr, numElems);
        
        v = (key.z >> bitshift) & 0x3;
        scatterAddr = in_histograms[groupId + v * numGroups] + localOffset.z;
        safe_store_int(key.z, (__global int*)out_keys, scatterAddr, numElems);
        safe_store_int(value.z, (__global int*)out_values, scatterAddr, numElems);
        
        v = (key.w >> bitshift) & 0x3;
        scatterAddr = in_histograms[groupId + v * numGroups] + localOffset.w;
        safe_store_int(key.w, (__global int*)out_keys, scatterAddr, numElems);
        safe_store_int(value.w, (__global int*)out_values, scatterAddr, numElems);
    }
}

__kernel void compact_int( __global int* in_predicate, __global int* in_address, 
                       __global int* in_input, uint in_size,
                       __global int* out_output)
{
    int global_id  = get_global_id(0);
    int group_id   = get_group_id(0);

    if (global_id < in_size)
    {
        if (in_predicate[global_id])
        {
            out_output[in_address[global_id]] = in_input[global_id];
        }
    }
}



