/*
 Banshee and all code, documentation, and other materials contained
 therein are:
 
 Copyright 2015 Dmitry Kozlov
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
#ifndef GRID_H
#define GRID_H

#include <memory>
#include <vector>
#include <atomic>

#include "../math/bbox.h"
#include "../primitive/primitive.h"

///< The class represents regular grid intersection accelerator
///<
class Grid : public Primitive
{
public:
    Grid()
    {
    }
    
    // Intersection test
    bool Intersect(ray& r, float& t, Intersection& isect) const;
    // Intersection check test
    bool Intersect(ray& r) const;
    // World space bounding box
    bbox Bounds() const;
    // Build function
    void Build(std::vector<Primitive*> const& prims);
    
#ifdef _DEBUG
    // Temporary stuff to dump statistics
    void DumpStatistics();
#endif

protected:
    // Build function
    virtual void BuildImpl(std::vector<Primitive*> const& prims);
    // Primitive binning
    virtual void BinPrimitives(std::vector<Primitive*> const& prims);
    // Helper methods
    // Position to voxel coords
    void PosToVoxel(float3 const& p, int voxel[3]) const;
    // Voxel coords to min corner position
    float3 VoxelToPos(int x, int y, int z) const;
    // Voxel coords to min corner position
    float  VoxelToPos(int x, int axis) const;
    // Voxel coords to plain index
    int VoxelPlainIndex(int x, int y, int z) const;
    
    // Grid voxel
    struct Voxel
    {
        // Primitives in this voxel (hope not too many)
        std::vector<Primitive*> prims;
        
#ifdef _DEBUG
        std::atomic<int> numvisits;
#endif
    };
    
    // Voxels
    std::vector<std::unique_ptr<Voxel> > voxels_;
    // Primitves owned by this instance
    std::vector<std::unique_ptr<Primitive> > primitive_storage_;
    // Primitves to intersect (includes refined prims)
    std::vector<Primitive*> primitives_;
    // Overall cached bounds
    bbox bounds_;
    // Grid resolution in each dimension
    int  gridres_[3];
    // Voxel sizes
    float3 voxelsize_;
    // Voxel size inverse
    float3 voxelsizeinv_;
    // Primitive indices for the voxels
    
private:
    Grid(Grid const&);
    Grid& operator = (Grid const&);
};

#endif // GRID_H