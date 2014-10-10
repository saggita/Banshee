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
__attribute__((reqd_work_group_size(64, 1, 1)))\
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
    type t = v1.w; v1.w = v2.w; v2.w += t;\
    t = v1.y; v1.y = v1.w; v1.w += t;\
    t = v2.y; v2.y = v2.w; v2.w += t;\
    t = v1.x; v1.x = v1.y; v1.y += t;\
    t = v2.x; v2.x = v2.y; v2.y += t;\
    t = v1.z; v1.z = v1.w; v1.w += t;\
    t = v2.z; v2.z = v2.w; v2.w += t;\
    safe_store_##type##4(v2, out_array, 2 * globalId + 1, numElems);\
    safe_store_##type##4(v1, out_array, 2 * globalId, numElems);\
}

#define DEFINE_SCAN_EXCLUSIVE_4_V1(type)\
__attribute__((reqd_work_group_size(64, 1, 1)))\
__kernel void scan_exclusive_##type##4##_v1(__global type##4 const* in_array, __global type##4* out_array, uint numElems, __local type* shmem)\
{\
int globalId  = get_global_id(0);\
int localId   = get_local_id(0);\
int groupSize = get_local_size(0);\
type##4 v1 = safe_load_##type##4(in_array, 2*globalId, numElems);\
type##4 v2 = safe_load_##type##4(in_array, 2*globalId + 1, numElems);\
shmem[localId] = v1.x + v1.y + v1.z + v1.w + v2.x + v2.y + v2.z + v2.w;\
barrier(CLK_LOCAL_MEM_FENCE);\
group_scan_exclusive_##type(localId, groupSize, shmem);\
type offset = shmem[localId];\
type t = v1.x; v1.x = offset; offset += t;\
t = v1.y; v1.y = offset; offset += t;\
t = v1.z; v1.z = offset; offset += t;\
t = v1.w; v1.w = offset; offset += t;\
t = v2.x; v2.x = offset; offset += t;\
t = v2.y; v2.y = offset; offset += t;\
t = v2.z; v2.z = offset; offset += t;\
v2.w = offset;\
safe_store_##type##4(v2, out_array, 2 * globalId + 1, numElems);\
safe_store_##type##4(v1, out_array, 2 * globalId, numElems);\
}

#define DEFINE_SCAN_EXCLUSIVE_PART_4(type)\
__attribute__((reqd_work_group_size(64, 1, 1)))\
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
    type t = v1.w; v1.w = v2.w; v2.w += t;\
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
    type##4 v1 = safe_load_##type##4(inout_array, globalId, numElems);\
    type    sum = in_sums[groupId >> 1];\
    v1.xyzw += sum;\
    safe_store_##type##4(v1, inout_array, globalId, numElems);\
}


//DEFINE_MAKE_4(int)
//DEFINE_MAKE_4(float)

DEFINE_SAFE_LOAD_4(int)
DEFINE_SAFE_LOAD_4(float)

DEFINE_SAFE_STORE_4(int)
DEFINE_SAFE_STORE_4(float)

DEFINE_GROUP_SCAN_EXCLUSIVE(int)
DEFINE_GROUP_SCAN_EXCLUSIVE(uint)
DEFINE_GROUP_SCAN_EXCLUSIVE(float)
DEFINE_GROUP_SCAN_EXCLUSIVE(short)

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
    
    barrier(CLK_LOCAL_MEM_FENCE);

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
short offset_4way(int val, int offset, short offset0, short offset1, short offset2, short offset3, short4 hist)
{
    switch ((val >> offset) & 3)
    {
        case 0:
            return offset0;
        case 1:
            return offset1 + hist.x;
        case 2:
            return offset2 + hist.x + hist.y;
        case 3:
            return offset3 + hist.x + hist.y + hist.z;
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
                            int4* localOffset,
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

    (*localOffset).x = offset_4way(val.x, offset, offset0.x, offset1.x, offset2.x, offset3.x, *histogram);
    (*localOffset).y = offset_4way(val.y, offset, offset0.y, offset1.y, offset2.y, offset3.y, *histogram);
    (*localOffset).z = offset_4way(val.z, offset, offset0.z, offset1.z, offset2.z, offset3.z, *histogram);
    (*localOffset).w = offset_4way(val.w, offset, offset0.w, offset1.w, offset2.w, offset3.w, *histogram);
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
__kernel void split4way(int bitshift, __global int4* in_array, uint numElems, __global int* out_histograms, __global int4* out_array,
                         __global int* out_local_histograms,
                         __global int4* out_debug_offset,
                         __local short* shmem)
{
    int globalId  = get_global_id(0);
    int localId   = get_local_id(0);
    int groupSize = get_local_size(0);
    int groupId   = get_group_id(0);
    int numGroups = get_global_size(0) / groupSize;

    /// Load single int4 value
    int4 val = safe_load_int4_intmax(in_array, globalId, numElems);

    int4 localOffset;
    short4 localHistogram;
    group_split_radix_2bits(localId, groupSize, bitshift, val, shmem, &localOffset,
        &localHistogram);

    barrier(CLK_LOCAL_MEM_FENCE);

    __local int* sharedData = (__local int*)shmem;
    __local int4* sharedData4 = (__local int4*)shmem;
    
    sharedData[localOffset.x] = val.x;
    sharedData[localOffset.y] = val.y;
    sharedData[localOffset.z] = val.z;
    sharedData[localOffset.w] = val.w;

    barrier(CLK_LOCAL_MEM_FENCE);

    // Now store to memory
    if (((globalId + 1) << 2) <= numElems)
    {
        out_array[globalId] = sharedData4[localId];
        out_debug_offset[globalId] = localOffset;
    }
    else
    {
        if ((globalId << 2) < numElems) out_array[globalId].x = sharedData4[localId].x;
        if ((globalId << 2) + 1 < numElems) out_array[globalId].y = sharedData4[localId].y;
        if ((globalId << 2) + 2 < numElems) out_array[globalId].z = sharedData4[localId].z;
    }

    if (localId == 0)
    {
        out_histograms[groupId] = localHistogram.x;
        out_histograms[groupId + numGroups] =  localHistogram.y;
        out_histograms[groupId + 2 * numGroups] = localHistogram.z;
        out_histograms[groupId + 3 * numGroups] =  localHistogram.w;
        
        out_local_histograms[groupId] = 0;
        out_local_histograms[groupId + numGroups] = localHistogram.x;
        out_local_histograms[groupId + 2 * numGroups] = localHistogram.x + localHistogram.y;
        out_local_histograms[groupId + 3 * numGroups] = localHistogram.x + localHistogram.y + localHistogram.z;
    }
}

// The kernel computes 16 bins histogram of the 256 input elements.
// The bin is determined by (in_array[tid] >> bitshift) & 0xF
__kernel void BitHistogram(
                           // Number of bits to shift
                           int bitshift,
                           // Input array
                           __global int4* in_array,
                           // Number of elements in input array
                           uint numelems,
                           // Output histograms in column layout
                           // [bin0_group0, bin0_group1, ... bin0_groupN, bin1_group0, bin1_group1, ... bin1_groupN, ...]
                           __global int* out_histogram
                           )
{
    // Num bins
    const int kNumBins = 16;

    // Histogram storage
    __local short histogram[kNumBins * 64];

    int globalid  = get_global_id(0);
    int localid   = get_local_id(0);
    int groupsize = get_local_size(0);
    int groupid   = get_group_id(0);
    int numgroups = get_global_size(0) / groupsize;

    /// Load single int4 value
    int4 value = safe_load_int4_intmax(in_array, globalid, numelems);

    /// Clear local histogram
    for (int i=0; i<kNumBins; ++i)
    {
        histogram[kNumBins*localid + i] = 0;
    }

    /// Sync
    barrier(CLK_LOCAL_MEM_FENCE);

    /// Handle value adding histogram bins
    /// for all 4 elements
    int bin = ((value.x >> bitshift) & 0xF);
    ++histogram[localid*kNumBins + bin];
    bin = ((value.y >> bitshift) & 0xF);
    ++histogram[localid*kNumBins + bin];
    bin = ((value.z >> bitshift) & 0xF);
    ++histogram[localid*kNumBins + bin];
    bin = ((value.w >> bitshift) & 0xF);
    ++histogram[localid*kNumBins + bin];

    /// Make sure LDS is up to date
    barrier(CLK_LOCAL_MEM_FENCE);

    if (localid < kNumBins)
    {
        short sum = 0;
        // TODO: perform reduction here if this is too slow
        for (int i = 0; i < 64; ++i)
        {
            sum += histogram[i * kNumBins + localid];
        }

        out_histogram[numgroups*localid + groupid] = sum;
    }

}




__kernel void ScatterKeys(// Number of bits to shift
                          int bitshift,
                          // Input ketys
                          __global int4* in_keys,
                          // Number of input keys
                          uint           numelems,
                          // Scanned histograms
                          __global int*  in_histograms,
                          // Output keys
                          __global int*  out_keys
                          )
{
    // Local memory for offsets counting
    __local uint histogram[64];
    __local int  keys[256];
    __local ushort totalhist[4];

    int globalid  = get_global_id(0);
    int localid   = get_local_id(0);
    int groupsize = get_local_size(0);
    int groupid   = get_group_id(0);
    int numgroups = get_global_size(0) / groupsize;

    // Load single int4 value
    int4 value = safe_load_int4_intmax(in_keys, globalid, numelems);
    
    // Clear the histogram
    histogram[localid] = 0;
    
    // Make sure everything is up to date
    barrier(CLK_LOCAL_MEM_FENCE);
    
    // Count local values
    uchar localhist[4] = {0, 0, 0, 0};
    int localvals[4] = {
        value.x,
        value.y,
        value.z,
        value.w
    };
    
    // Do 2 bits per pass
    for (int bit = 0; bit <= 2; bit += 2)
    {
        // Clear histogram
        for (int i=0; i<4; ++i)
        {
            localhist[i] = 0;
        }
    
        // Count histogram
        for (int i=0; i<4; ++i)
        {
            ++localhist[((localvals[i] >> bitshift) >> bit) & 0x3];
        }
    
        // Pack the histogram
        uint packed_key =
        (localhist[3] << 24) |
        (localhist[2] << 16) |
        (localhist[1] << 8 ) |
        (localhist[0]);
    
        // Put into LDS
        histogram[localid] = packed_key;
    
        // Make sure everything is up to date
        barrier(CLK_LOCAL_MEM_FENCE);
    
        // Calc total hist
        // TODO: temporary code, optimize later
        if (localid < 4)
        {
            totalhist[localid] = 0;
        
            for (int i=0; i<64; ++i)
            {
                uchar val = (histogram[i] >> (localid * 8)) & 0xFF;
                totalhist[localid] += val;
            }
        }
    
        // Make sure everything is up to date
        barrier(CLK_LOCAL_MEM_FENCE);
    
        // TODO: optimize this, very slow
        if (localid == 0)
        {
            totalhist[3] = totalhist[2] + totalhist[1] + totalhist[0];
            totalhist[2] = totalhist[1] + totalhist[0];
            totalhist[1] = totalhist[0];
            totalhist[0] = 0;
        }
    
        // Make sure everything is up to date
        barrier(CLK_LOCAL_MEM_FENCE);
    
        // Scan the histogram in LDS with 4-way plus scan
        group_scan_exclusive_uint(localid, groupsize, histogram);
    
        // Load value back
        packed_key = histogram[localid];
    
        // Unpack the value
        localhist[0] = (uchar)((packed_key)       & 0xFF);
        localhist[1] = (uchar)((packed_key >> 8)  & 0xFF);
        localhist[2] = (uchar)((packed_key >> 16) & 0xFF);
        localhist[3] = (uchar)((packed_key >> 24) & 0xFF);
    
        // Pull the data to corresponding offsets to LDS
        for (int i=0; i<4; ++i)
        {
            uchar current = ((localvals[i] >> bitshift) >> bit) & 0x3;
            keys[totalhist[current] + localhist[current]] = localvals[i];
            localhist[current]++;
        }
    
        // Make sure everything is up to date
        barrier(CLK_LOCAL_MEM_FENCE);
        
        // Reload values back to registers for the second bit pass
        for (int i=0; i<4; ++i)
        {
            localvals[i] = keys[localid*4 + i];
        }
    }
    
    // Clear LDS
    histogram[localid] = 0;
    
    // Make sure everything is up to date
    barrier(CLK_LOCAL_MEM_FENCE);
    
    // TODO: Opimize this away
    // Reconstruct 16 bins histogram
    for (int i=0; i<4; ++i)
    {
        int bin = (localvals[i] >> bitshift) & 0xF;
        atomic_inc(&histogram[bin]);
    }
    
    // Make sure everything is up to date
    barrier(CLK_LOCAL_MEM_FENCE);
    
    // Scan the histogram
    if (localid == 0)
    {
        int sum = 0;
        for (int i=0; i<16; ++i)
        {
            int t = histogram[i];
            histogram[i] = sum;
            sum += t;
        }
    }
    
    // Make sure everything is up to date
    barrier(CLK_LOCAL_MEM_FENCE);
    
    
    // Put data back to global memory
    for (int i=0; i<4; ++i)
    {
        int v = (localvals[i] >> bitshift) & 0xF;
        int offset = in_histograms[groupid + v * numgroups] + (localid*4 + i) - histogram[v];
        if (offset < numelems)
        {
            out_keys[offset] = localvals[i];
        }
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


__kernel void scatter_keys_k(int bitshift,
                           __global int4* in_keys,
                           uint numElems,
                           __global int* in_histograms,
                           __global int* in_local_histograms,
                           __global int4* out_keys
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
        
        uchar v = (key.x >> bitshift) & 0x3;
        int scatterAddr = in_histograms[groupId + v * numGroups] +
        4 * localId - in_local_histograms[groupId + v * numGroups];
        safe_store_int(key.x, (__global int*)out_keys, scatterAddr, numElems);
        
        v = (key.y >> bitshift) & 0x3;
        scatterAddr = in_histograms[groupId + v * numGroups] +
        4 * localId + 1 - in_local_histograms[groupId + v * numGroups];
        safe_store_int(key.y, (__global int*)out_keys, scatterAddr, numElems);
        
        v = (key.z >> bitshift) & 0x3;
        scatterAddr = in_histograms[groupId + v * numGroups] +
        4 * localId + 2 - in_local_histograms[groupId + v * numGroups];
        safe_store_int(key.z, (__global int*)out_keys, scatterAddr, numElems);
        
        v = (key.w >> bitshift) & 0x3;
        scatterAddr = in_histograms[groupId + v * numGroups] +
        4 * localId + 3 - in_local_histograms[groupId + v * numGroups];
        safe_store_int(key.w, (__global int*)out_keys, scatterAddr, numElems);
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

__kernel void copy(__global int4* in_input, 
                                   uint  in_size,
                          __global int4* out_output)
{
    int global_id  = get_global_id(0);
    int4 value = safe_load_int4(in_input, global_id, in_size);
    safe_store_int4(value, out_output, global_id, in_size);
}


#define FLAG(x) (flags[(x)] & 0x1)
#define FLAG_COMBINED(x) (flags[(x)])
#define FLAG_ORIG(x) ((flags[(x)] >> 1) & 0x1)

void group_segmented_scan_exclusive_int(
                                        int localId,
                                        int groupSize,
                                        __local int* shmem,
                                        __local char* flags
                                        )
{
    for (int stride = 1; stride <= (groupSize >> 1); stride <<= 1)
    {
        if (localId < groupSize/(2*stride))
        {
            if (FLAG(2*(localId + 1)*stride-1) == 0)
            {
                shmem[2*(localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1] + shmem[(2*localId + 1)*stride-1];
            }
        
            FLAG_COMBINED(2*(localId + 1)*stride-1) = FLAG_COMBINED(2*(localId + 1)*stride-1) | FLAG((2*localId + 1)*stride-1);
        }
        
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    
    if (localId == 0)
        shmem[groupSize - 1] = 0;
    
    barrier(CLK_LOCAL_MEM_FENCE);

    for (int stride = (groupSize >> 1); stride > 0; stride >>= 1)
    {
        if (localId < groupSize/(2*stride))
        {
            int temp = shmem[(2*localId + 1)*stride-1];
            shmem[(2*localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1];
            
            // optimize with a conditional = operator
            if (FLAG_ORIG((2*localId + 1)*stride) == 1)
            {
                shmem[2*(localId + 1)*stride-1] = 0;
            }
            else if (FLAG((2*localId + 1)*stride-1) == 1)
            {
                shmem[2*(localId + 1)*stride-1] = temp;
            }
            else
            {
                shmem[2*(localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1] + temp;
            }
            
            FLAG_COMBINED((2*localId + 1)*stride-1) = FLAG_COMBINED((2*localId + 1)*stride-1) & 2;
        }
        
        barrier(CLK_LOCAL_MEM_FENCE);
    }
}

void group_segmented_scan_exclusive_int_nocut(
                                        int localId,
                                        int groupSize,
                                        __local int* shmem,
                                        __local char* flags
                                        )
{
    for (int stride = 1; stride <= (groupSize >> 1); stride <<= 1)
    {
        if (localId < groupSize/(2*stride))
        {
            if (FLAG(2*(localId + 1)*stride-1) == 0)
            {
                shmem[2*(localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1] + shmem[(2*localId + 1)*stride-1];
            }
            
            FLAG_COMBINED(2*(localId + 1)*stride-1) = FLAG_COMBINED(2*(localId + 1)*stride-1) | FLAG((2*localId + 1)*stride-1);
        }
        
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    
    if (localId == 0)
        shmem[groupSize - 1] = 0;
    
    barrier(CLK_LOCAL_MEM_FENCE);
    
    for (int stride = (groupSize >> 1); stride > 0; stride >>= 1)
    {
        if (localId < groupSize/(2*stride))
        {
            int temp = shmem[(2*localId + 1)*stride-1];
            shmem[(2*localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1];
            
            if (FLAG((2*localId + 1)*stride-1) == 1)
            {
                shmem[2*(localId + 1)*stride-1] = temp;
            }
            else
            {
                shmem[2*(localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1] + temp;
            }
            
            FLAG_COMBINED((2*localId + 1)*stride-1) = FLAG_COMBINED((2*localId + 1)*stride-1) & 2;
        }
        
        barrier(CLK_LOCAL_MEM_FENCE);
    }
}


void group_segmented_scan_exclusive_int_part(
                                        int localId,
                                        int groupId,
                                        int groupSize,
                                        __local int* shmem,
                                        __local char* flags,
                                        __global int* part_sums,
                                        __global int* part_flags
                                        )
{
    for (int stride = 1; stride <= (groupSize >> 1); stride <<= 1)
    {
        if (localId < groupSize/(2*stride))
        {
            if (FLAG(2*(localId + 1)*stride-1) == 0)
            {
                shmem[2*(localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1] + shmem[(2*localId + 1)*stride-1];
            }
            
            FLAG_COMBINED(2*(localId + 1)*stride-1) = FLAG_COMBINED(2*(localId + 1)*stride-1) | FLAG((2*localId + 1)*stride-1);
        }
        
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    
    if (localId == 0)
    {
        part_sums[groupId] = shmem[groupSize - 1];
        part_flags[groupId] = FLAG(groupSize - 1);
        shmem[groupSize - 1] = 0;
    }
    
    barrier(CLK_LOCAL_MEM_FENCE);
    
    for (int stride = (groupSize >> 1); stride > 0; stride >>= 1)
    {
        if (localId < groupSize/(2*stride))
        {
            int temp = shmem[(2*localId + 1)*stride-1];
            shmem[(2*localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1];
            
            // optimize with a conditional = operator
            if (FLAG_ORIG((2*localId + 1)*stride) == 1)
            {
                shmem[2*(localId + 1)*stride-1] = 0;
            }
            else if (FLAG((2*localId + 1)*stride-1) == 1)
            {
                shmem[2*(localId + 1)*stride-1] = temp;
            }
            else
            {
                shmem[2*(localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1] + temp;
            }
            
            FLAG_COMBINED((2*localId + 1)*stride-1) = FLAG_COMBINED((2*localId + 1)*stride-1) & 2;
        }
        
        barrier(CLK_LOCAL_MEM_FENCE);
    }
}


__kernel void segmented_scan_exclusive_int_nocut(__global int const* in_array,
                                           __global int const* in_segment_heads_array,
                                           __global int* out_array,
                                           __local int* shmem)
{
    int globalId  = get_global_id(0);
    int localId   = get_local_id(0);
    int groupSize = get_local_size(0);
    int groupId   = get_group_id(0);
    
    __local int* keys = shmem;
    __local char* flags = (__local char*)(keys + groupSize);
    
    int4 v1 = in_array[2*globalId];
    int4 v2 = in_array[2*globalId + 1];
    int4 f1 = in_segment_heads_array[2*globalId];
    int4 f2 = in_segment_heads_array[2*globalId + 1];
    
    keys[localId] = in_array[globalId];
    flags[localId] = in_segment_heads_array[globalId] ? 3 : 0;
    
    barrier(CLK_LOCAL_MEM_FENCE);
    
    group_segmented_scan_exclusive_int_nocut(localId, groupSize, keys, flags);
    
    out_array[globalId] = keys[localId];
}

__kernel void segmented_scan_exclusive_int(__global int const* in_array,
                                           __global int const* in_segment_heads_array,
                                           __global int* out_array,
                                           __local int* shmem)
{
    int globalId  = get_global_id(0);
    int localId   = get_local_id(0);
    int groupSize = get_local_size(0);
    int groupId   = get_group_id(0);
    
    __local int* keys = shmem;
    __local char* flags = (__local char*)(keys + groupSize);
    
    int4 v1 = in_array[2*globalId];
    int4 v2 = in_array[2*globalId + 1];
    int4 f1 = in_segment_heads_array[2*globalId];
    int4 f2 = in_segment_heads_array[2*globalId + 1];
    
    keys[localId] = in_array[globalId];
    flags[localId] = in_segment_heads_array[globalId] ? 3 : 0;
    
    barrier(CLK_LOCAL_MEM_FENCE);
    
    group_segmented_scan_exclusive_int(localId, groupSize, keys, flags);
    
    out_array[globalId] = keys[localId];
}

__kernel void segmented_scan_exclusive_int_part(__global int const* in_array,
                                           __global int const* in_segment_heads_array,
                                           __global int* out_array,
                                           __global int* out_part_sums,
                                           __global int* out_part_flags,
                                           __local int* shmem)
{
    int globalId  = get_global_id(0);
    int localId   = get_local_id(0);
    int groupSize = get_local_size(0);
    int groupId   = get_group_id(0);
    
    __local int* keys = shmem;
    __local char* flags = (__local char*)(keys + groupSize);
    
    int4 v1 = in_array[2*globalId];
    int4 v2 = in_array[2*globalId + 1];
    int4 f1 = in_segment_heads_array[2*globalId];
    int4 f2 = in_segment_heads_array[2*globalId + 1];
    
    keys[localId] = in_array[globalId];
    flags[localId] = in_segment_heads_array[globalId] ? 3 : 0;
    
    barrier(CLK_LOCAL_MEM_FENCE);
    
    group_segmented_scan_exclusive_int_part(localId, groupId, groupSize, keys, flags, out_part_sums, out_part_flags);
    
    out_array[globalId] = keys[localId];
}

__kernel void segmented_distribute_part_sum_int(
                                       __global int* inout_array,
                                       __global int* in_flags,
                                       __global int* in_sums
                                       )
{
    int globalId  = get_global_id(0);
    int localId   = get_local_id(0);
    int groupSize = get_local_size(0);
    int groupId   = get_group_id(0);
    
    int sum = in_sums[groupId];
    //inout_array[globalId] += sum;
    
    if (localId == 0)
    {
        for (int i = 0; in_flags[globalId + i] == 0 && i < groupSize; ++i)
        {
            inout_array[globalId + i] += sum;
        }
    }
}
                                       



