#include "grid.h"

#include "../math/mathutils.h"

#include <algorithm>
#include <thread>
#include <stack>
#include <numeric>
#include <cassert>
#include <cmath>
#include <iostream>

static const int kMaxVoxels = 128;
//static const int kSubdivisionTreshold = 64;

void Grid::PosToVoxel(float3 const& p, int voxel[3]) const
{
    voxel[0] = clamp((int)std::floor((p.x - bounds_.pmin.x) * voxelsizeinv_.x), 0, gridres_[0]-1);
    voxel[1] = clamp((int)std::floor((p.y - bounds_.pmin.y) * voxelsizeinv_.y), 0, gridres_[1]-1);
    voxel[2] = clamp((int)std::floor((p.z - bounds_.pmin.z) * voxelsizeinv_.z), 0, gridres_[2]-1);
}

float3 Grid::VoxelToPos(int x, int y, int z) const
{
    return float3(bounds_.pmin.x + x * voxelsize_.x,
                  bounds_.pmin.y + y * voxelsize_.y,
                  bounds_.pmin.z + z * voxelsize_.z);
}

float  Grid::VoxelToPos(int x, int axis) const
{
    return bounds_.pmin[axis] + x * voxelsize_[axis];
}

int Grid::VoxelPlainIndex(int x, int y, int z) const
{
    return z * gridres_[0] * gridres_[1] + y * gridres_[0] + x;
}

void Grid::Build(std::vector<Primitive*> const& prims)
{
    // Store all the primitives in storage array
    primitive_storage_.resize(prims.size());
    
    // Reserve
    std::vector<Primitive*> tempprims;
    tempprims.reserve(prims.size());
    primitives_.reserve(prims.size());
    for (int i=0; i<prims.size(); ++i)
    {
        primitive_storage_[i] = std::unique_ptr<Primitive>(prims[i]);
        
        // Refine the primitves
        // TODO: only a single level of indirection for now
        if (prims[i]->intersectable())
        {
            tempprims.push_back(prims[i]);
        }
        else
        {
            prims[i]->Refine(tempprims);
        }
        
        // Calc bbox
        bounds_ = bboxunion(bounds_, prims[i]->Bounds());
    }
    
    // Enlarge bounding box a bit
    bounds_.grow(bounds_.pmin + 1.05f * bounds_.extents());
    bounds_.grow(bounds_.pmin - 0.05f * bounds_.extents());
    
    BuildImpl(tempprims);
}

void Grid::BuildImpl(std::vector<Primitive*> const& prims)
{
    // Number of primitives (all refined)
    int numprims = (int)prims.size();
    
    // Find scene spatial extent
    float3 extents = bounds_.extents();
    
    // Find scene maximum extent dimension
    int maxextent = bounds_.maxdim();
    
    // Grid size is proportional to cubic root
    int gridres = (int)std::powf((float)numprims, 1.f / 3.f);
    
    // Find number of voxels per unit distance
    float voxelperunit = gridres / extents[maxextent];
    
    // Initialize grid resolution now
    for (int i=0; i<3; ++i)
    {
        gridres_[i] = clamp((int)std::ceil(extents[i] * voxelperunit), 1, kMaxVoxels);
        voxelsize_[i] = extents[i] / gridres_[i];
        voxelsizeinv_[i] = 1.f / voxelsize_[i];
    }
    
    // Reserve memory
    voxels_.resize(gridres_[0] * gridres_[1] * gridres_[2]);
    
    // Start primitive binning
    BinPrimitives(prims);
}

void Grid::BinPrimitives(std::vector<Primitive*> const& prims)
{
    int minvoxel[3];
    int maxvoxel[3];
    
    for (int i=0; i<(int)prims.size(); ++i)
    {
        // Get bounding box of the primitive
        bbox bounds = prims[i]->Bounds();
        
        // Find voxels for min and max points
        for (int j=0; j<3; ++j)
        {
            minvoxel[j] = clamp((int)std::floor((bounds.pmin[j] - bounds_.pmin[j]) * voxelsizeinv_[j]), 0, gridres_[j] - 1);
            maxvoxel[j] = clamp((int)std::floor((bounds.pmax[j] - bounds_.pmin[j]) * voxelsizeinv_[j]), 0, gridres_[j] - 1);
        }
        
        // Add primitive to all voxels it potentially overlaps
        // TODO: add specific method to primitive to test vs voxel
        for (int x=minvoxel[0]; x<=maxvoxel[0]; ++x)
            for (int y=minvoxel[1]; y<=maxvoxel[1]; ++y)
                for (int z=minvoxel[2]; z<=maxvoxel[2]; ++z)
                {
                    int idx = VoxelPlainIndex(x,y,z);
                    
                    if (!voxels_[idx])
                    {
                        voxels_[idx].reset(new Voxel());
                    }
                    
                    voxels_[idx]->prims.push_back(prims[i]);
                }
    }
    
#ifdef _DEBUG
    // Number of occupied cells
    int numoccupied = 0;
    // Max number of primitives
    int maxprims = 0;
    // Collect some statistics
    for (int z=0; z<gridres_[2]; ++z)
        for (int y=0; y<gridres_[1]; ++y)
            for (int x=0; x<gridres_[0]; ++x)
            {
                //
                int idx = VoxelPlainIndex(x,y,z);
                
                //
                if (voxels_[idx])
                {
                    ++numoccupied;
                    maxprims = std::max(maxprims, (int)voxels_[idx]->prims.size());
                }
            }
    
    std::cout << "Grid resolution: " << gridres_[0] << " " << gridres_[1] << " " << gridres_[2] << "\n";
    std::cout << "Number of occupied voxels: " << numoccupied << " (" << (int)((float)numoccupied/voxels_.size() * 100.f) << "%)\n";
    std::cout << "Max primitives per voxel: " << maxprims << "\n";
    std::cout << "Avg primitives per occupied voxel: " << (float)prims.size() / numoccupied << "\n";
#endif
        
}

// Intersection test
bool Grid::Intersect(ray& r, float& t, Intersection& isect) const
{
    // Precalc inv ray dir for bbox testing
    float3 invrd = float3(1.f / r.d.x, 1.f / r.d.y, 1.f / r.d.z);
    // Precalc ray direction signs: 1 if negative, 0 otherwise
    int dirneg[3] =
    {
        r.d.x < 0.f ? 1 : 0,
        r.d.y < 0.f ? 1 : 0,
        r.d.z < 0.f ? 1 : 0
    };
    
    // Make sure ray intersects with the bounding box
    float2 range;
    
    if (!intersects(r, invrd, bounds_, dirneg, range))
    {
        return false;
    }
    
    // Parametric distance
    float rayt = range.x;
    
    // Compute staring point
    float3 isectpoint = r(rayt);
    
    // Voxel position
    int voxel[3];
    
    // Perpare the stepping data
    float nexthit[3], dt[3];
    int step[3], out[3];
    
    for (int axis = 0; axis < 3; ++axis)
    {
        PosToVoxel(isectpoint, voxel);
        
        assert(voxel[0] < gridres_[0] && voxel[1] < gridres_[1] && voxel[2] < gridres_[2]);
        
        if (r.d[axis] >= 0)
        {
            // Next hit along this axis in parametric units
            nexthit[axis] = rayt + (VoxelToPos(voxel[axis]+1, axis) - isectpoint[axis]) / r.d[axis];
            
            // Parametric voxel size with respect to the ray
            dt[axis] = voxelsize_[axis] / r.d[axis];
            
            // Step forward
            step[axis] = 1;
            
            // Last voxel
            // TODO: account for ray maxt
            out[axis] = gridres_[axis];
        }
        else
        {
            // Next hit along this axis in parametric units
            nexthit[axis] = rayt + (VoxelToPos(voxel[axis], axis) - isectpoint[axis]) / r.d[axis];
            
            // Parametric voxel size with respect to the ray
            dt[axis] = - voxelsize_[axis] / r.d[axis];
            
            // Step forward
            step[axis] = -1;
            
            // Last voxel
            // TODO: account for ray maxt
            out[axis] = -1;
        }
    }
    
    bool hit = false;
    for(;;)
    {
        // Check current voxel
        int idx = VoxelPlainIndex(voxel[0], voxel[1], voxel[2]);
        
        if (voxels_[idx])
        {
            // Check for intersection
            for (int i=0; i<(int)voxels_[idx]->prims.size(); ++i)
            {
                if (voxels_[idx]->prims[i]->Intersect(r, t, isect))
                {
                    r.t.y = t;
                    hit  = true;
                    
#ifdef _DEBUG
                    ++voxels_[idx]->numvisits;
#endif
                }
            }
        }
        
        // Advance to next one
        int bits = ((nexthit[0] < nexthit[1]) << 2) +
        ((nexthit[0] < nexthit[2]) << 1) +
        ((nexthit[1] < nexthit[2]));
        
        const int cmptoaxis[8] = { 2, 1, 2, 1, 2, 2, 0, 0 };
        
        // Choose axis
        int stepaxis = cmptoaxis[bits];
        
        // Early out
        if (r.t.y < nexthit[stepaxis])
        {
            break;
        }
        
        // Advance current position
        voxel[stepaxis] += step[stepaxis];
        
        // Exit condition
        if (voxel[stepaxis] == out[stepaxis])
        {
            break;
        }
        
        assert(voxel[0] < gridres_[0] && voxel[1] < gridres_[1] && voxel[2] < gridres_[2]);
        
        // Advance next hit
        nexthit[stepaxis] += dt[stepaxis];
    }

    
    return hit;
}

// Intersection check test
bool Grid::Intersect(ray& r) const
{
    // Precalc inv ray dir for bbox testing
    float3 invrd = float3(1.f / r.d.x, 1.f / r.d.y, 1.f / r.d.z);
    // Precalc ray direction signs: 1 if negative, 0 otherwise
    int dirneg[3] =
    {
        r.d.x < 0.f ? 1 : 0,
        r.d.y < 0.f ? 1 : 0,
        r.d.z < 0.f ? 1 : 0
    };
    
    // Make sure ray intersects with the bounding box
    float2 range;
    if (!intersects(r, invrd, bounds_, dirneg, range))
    {
        return false;
    }
    
    // Parametric distance
    float rayt = range.x;
    
    // Compute staring point
    float3 isectpoint = r(rayt);
    
    // Voxel position
    int voxel[3];
    
    // Perpare the stepping data
    float nexthit[3], dt[3];
    int step[3], out[3];
    
    for (int axis = 0; axis < 3; ++axis)
    {
        PosToVoxel(isectpoint, voxel);
        
        if (r.d[axis] >= 0)
        {
            // Next hit along this axis in parametric units
            nexthit[axis] = rayt + (VoxelToPos(voxel[axis]+1, axis) - isectpoint[axis]) / r.d[axis];
            
            // Parametric voxel size with respect to the ray
            dt[axis] = voxelsize_[axis] / r.d[axis];
            
            // Step forward
            step[axis] = 1;
            
            // Last voxel
            // TODO: account for ray maxt
            out[axis] = gridres_[axis];
        }
        else
        {
            // Next hit along this axis in parametric units
            nexthit[axis] = rayt + (VoxelToPos(voxel[axis], axis) - isectpoint[axis]) / r.d[axis];
            
            // Parametric voxel size with respect to the ray
            dt[axis] = - voxelsize_[axis] / r.d[axis];
            
            // Step forward
            step[axis] = -1;
            
            // Last voxel
            // TODO: account for ray maxt
            out[axis] = -1;
        }
    }
    
    for(;;)
    {
        // Check current voxel
        int idx = VoxelPlainIndex(voxel[0], voxel[1], voxel[2]);
        
        if (voxels_[idx])
        {
            // Check for intersection
            for (int i=0; i<(int)voxels_[idx]->prims.size(); ++i)
            {
                if (voxels_[idx]->prims[i]->Intersect(r))
                {
                    return true;
                }
            }
        }
        
        // Advance to next one
        int bits = ((nexthit[0] < nexthit[1]) << 2) +
        ((nexthit[0] < nexthit[2]) << 1) +
        ((nexthit[1] < nexthit[2]));
        
        const int cmptoaxis[8] = { 2, 1, 2, 1, 2, 2, 0, 0 };
        
        // Choose axis
        int stepaxis = cmptoaxis[bits];
        
        // Early out
        if (r.t.y < nexthit[stepaxis])
        {
            break;
        }
        
        // Advance current position
        voxel[stepaxis] += step[stepaxis];
        
        // Exit condition
        if (voxel[stepaxis] == out[stepaxis])
        {
            break;
        }
        
        // Advance next hit
        nexthit[stepaxis] += dt[stepaxis];
    }
    
    
    return false;
}

// World space bounding box
bbox Grid::Bounds() const
{
    return bounds_;
}

#ifdef _DEBUG
void Grid::DumpStatistics()
{
    // Number of occupied cells
    int numoccupied = 0;
    // Max number of primitives
    int maxvisits = 0;
    // Total visits
    int numvisits = 0;
    // Collect some statistics
    for (int z=0; z<gridres_[2]; ++z)
        for (int y=0; y<gridres_[1]; ++y)
            for (int x=0; x<gridres_[0]; ++x)
            {
                // Get plain voxel index
                int idx = VoxelPlainIndex(x,y,z);
                
                // Increment visits counter
                if (voxels_[idx])
                {
                    numoccupied++;
                    maxvisits = std::max(maxvisits, (int)voxels_[idx]->numvisits);
                    numvisits += (int) voxels_[idx]->numvisits;
                }
            }
    
    std::cout << "Grid resolution: " << gridres_[0] << " " << gridres_[1] << " " << gridres_[2] << "\n";
    std::cout << "Number of occupied voxels: " << numoccupied << " (" << (int)((float)numoccupied/voxels_.size() * 100.f) << "%)\n";
    std::cout << "Max visits per voxel: " << maxvisits << "\n";
    std::cout << "Avg visits per occupied voxel: " << (float)numvisits/numoccupied << "\n";
}
#endif
