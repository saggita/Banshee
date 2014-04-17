//
//  BBox.h
//  BVHOQ
//
//  Created by dmitryk on 07.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#ifndef __BVHOQ__bbox__
#define __BVHOQ__bbox__

#include <iostream>
#include <algorithm>
#include <limits>

#include "CommonTypes.h"

_ALIGNED_CLASS(16) BBox
{
public:
    BBox();
	BBox(vector3opt const& p);
	BBox(vector3opt const& pmin, vector3opt const& pmax);
	
	vector3opt GetCenter() const;
	vector3opt GetExtents() const;
	
	bool Contains(vector3opt const& p) const;
    
	int   GetMaxDim() const;
	float GetSurfaceArea() const;

    inline void* operator new[](size_t x) { return _mm_malloc(x, 16); }
    inline void  operator delete[](void* x) { if (x) _mm_free(x); }

	vector3opt min;
	vector3opt max;
};

inline BBox BBoxUnion(BBox const& box1, BBox const& box2)
{	
	return BBox(vmin(box1.min, box2.min), vmax(box1.max, box2.max));
}

inline BBox BBoxIntersection(BBox const& box1, BBox const& box2)
{	
	return BBox(vmax(box1.min, box2.min), vmin(box1.max, box2.max));
}

inline bool Intersects(BBox const& box1, BBox const& box2)
{
	vector3opt b1Center = box1.GetCenter();
	vector3opt b1Radius = 0.5f * box1.GetExtents();
	vector3opt b2Center = box2.GetCenter();
	vector3opt b2Radius = 0.5f * box2.GetExtents();
	
	return abs(b2Center.x - b1Center.x) < b1Radius.x + b2Radius.x &&
		   abs(b2Center.y - b1Center.y) < b1Radius.y + b2Radius.y &&
		   abs(b2Center.x - b1Center.z) < b1Radius.z + b2Radius.z;
}

inline bool Contains(BBox const& box1, BBox const& box2)
{
	return box1.Contains(box2.min) && box1.Contains(box2.max);
}


inline vector3opt BBox::GetExtents() const
{
    return max - min;
}

inline vector3opt BBox::GetCenter() const
{
    return 0.5f * (max + min);
}

inline bool BBox::Contains(vector3opt const& p) const
{
    vector3opt radius = 0.5f * GetExtents();
    return abs(GetCenter().x - p.x) <= radius.x &&
        abs(GetCenter().y - p.y) <= radius.y &&
        abs(GetCenter().z - p.z) <= radius.z;
}

inline int BBox::GetMaxDim() const
{
    vector3opt ext = GetExtents();

    if (ext.x >= ext.y && ext.x >= ext.z)
        return 0;
    if (ext.y >= ext.x && ext.y >= ext.z)
        return 1;
    if (ext.z >= ext.x && ext.z >= ext.y)
        return 2;

    return 0;
}

inline float BBox::GetSurfaceArea() const
{
    vector3opt extents = GetExtents();
    return 2 * (extents.x * extents.y + extents.x * extents.z + extents.y * extents.z);
}

inline BBox::BBox()
: min(vector3opt(std::numeric_limits<float>::max(),
std::numeric_limits<float>::max(),
std::numeric_limits<float>::max()))
, max(vector3opt(-std::numeric_limits<float>::max(),
-std::numeric_limits<float>::max(),
-std::numeric_limits<float>::max()))
{

}

inline BBox::BBox(vector3opt const& p)
: min(p)
, max(p)
{

}

inline BBox::BBox(vector3opt const& p1, vector3opt const& p2)
: min(vmin(p1, p2))
, max(vmax(p1, p2))
{
}

#endif /* defined(__BVHOQ__bbox__) */
