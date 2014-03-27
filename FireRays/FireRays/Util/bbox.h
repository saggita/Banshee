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

class BBox
{
public:
    BBox();
	BBox(vector3 const& p);
	BBox(vector3 const& pmin, vector3 const& pmax);
	
	vector3& GetMaxPoint();
	vector3& GetMinPoint();
	
	vector3 const& GetMaxPoint() const;
	vector3 const& GetMinPoint() const;
	
	vector3 GetCenter() const;
	vector3 GetExtents() const;
	
	bool Contains(vector3 const& p) const;
    
	int   GetMaxDim() const;
	float GetSurfaceArea() const;
	
private:
	vector3 minPoint_;
	vector3 maxPoint_;
};

inline BBox BBoxUnion(BBox const& box1, BBox const& box2)
{
	vector3 pmin(std::min(box1.GetMinPoint().x(), box2.GetMinPoint().x()), std::min(box1.GetMinPoint().y(), box2.GetMinPoint().y()), std::min(box1.GetMinPoint().z(), box2.GetMinPoint().z()));
	vector3 pmax(std::max(box1.GetMaxPoint().x(), box2.GetMaxPoint().x()), std::max(box1.GetMaxPoint().y(), box2.GetMaxPoint().y()), std::max(box1.GetMaxPoint().z(), box2.GetMaxPoint().z()));
	
	return BBox(pmin, pmax);
}

inline BBox BBoxIntersection(BBox const& box1, BBox const& box2)
{
	vector3 pmin(std::max(box1.GetMinPoint().x(), box2.GetMinPoint().x()), std::max(box1.GetMinPoint().y(), box2.GetMinPoint().y()), std::max(box1.GetMinPoint().z(), box2.GetMinPoint().z()));
    
	vector3 pmax(std::min(box1.GetMaxPoint().x(), box2.GetMaxPoint().x()), std::min(box1.GetMaxPoint().y(), box2.GetMaxPoint().y()), std::min(box1.GetMaxPoint().z(), box2.GetMaxPoint().z()));
	
	return BBox(pmin, pmax);
}

inline bool Intersects(BBox const& box1, BBox const& box2)
{
	vector3 b1Center = box1.GetCenter();
	vector3 b1Radius = 0.5f * box1.GetExtents();
	vector3 b2Center = box2.GetCenter();
	vector3 b2Radius = 0.5f * box2.GetExtents();
	
	return abs(b2Center.x() - b1Center.x()) < b1Radius.x() + b2Radius.x() &&
		   abs(b2Center.y() - b1Center.y()) < b1Radius.y() + b2Radius.y() &&
		   abs(b2Center.x() - b1Center.z()) < b1Radius.z() + b2Radius.z();
}

inline bool Contains(BBox const& box1, BBox const& box2)
{
	return box1.Contains(box2.GetMinPoint()) && box1.Contains(box2.GetMaxPoint());
}

inline vector3& BBox::GetMaxPoint()
{
    return maxPoint_;
}

inline vector3& BBox::GetMinPoint()
{
    return minPoint_;
}

inline vector3 const& BBox::GetMaxPoint() const
{
    return maxPoint_;
}

inline vector3 const& BBox::GetMinPoint() const
{
    return minPoint_;
}

inline vector3 BBox::GetExtents() const
{
    return maxPoint_ - minPoint_;
}

inline vector3 BBox::GetCenter() const
{
    return 0.5f * (maxPoint_ + minPoint_);
}

inline bool BBox::Contains(vector3 const& p) const
{
    vector3 radius = 0.5f * GetExtents();
    return abs(GetCenter().x() - p.x()) <= radius.x() &&
        abs(GetCenter().y() - p.y()) <= radius.y() &&
        abs(GetCenter().z() - p.z()) <= radius.z();
}

inline int BBox::GetMaxDim() const
{
    vector3 ext = GetExtents();

    if (ext.x() >= ext.y() && ext.x() >= ext.z())
        return 0;
    if (ext.y() >= ext.x() && ext.y() >= ext.z())
        return 1;
    if (ext.z() >= ext.x() && ext.z() >= ext.y())
        return 2;

    return 0;
}

inline float BBox::GetSurfaceArea() const
{
    return 2 * (GetExtents().x() * GetExtents().y() + GetExtents().x() * GetExtents().z() + GetExtents().y() * GetExtents().z());
}

inline BBox::BBox()
: minPoint_(vector3(std::numeric_limits<float>::max(),
std::numeric_limits<float>::max(),
std::numeric_limits<float>::max()))
, maxPoint_(vector3(-std::numeric_limits<float>::max(),
-std::numeric_limits<float>::max(),
-std::numeric_limits<float>::max()))
{

}

inline BBox::BBox(vector3 const& p)
: minPoint_(p)
, maxPoint_(p)
{

}

inline BBox::BBox(vector3 const& p1, vector3 const& p2)
: minPoint_(std::min(p1.x(), p2.x()), std::min(p1.y(), p2.y()), std::min(p1.z(), p2.z()))
, maxPoint_(std::max(p1.x(), p2.x()), std::max(p1.y(), p2.y()), std::max(p1.z(), p2.z()))
{
}

#endif /* defined(__BVHOQ__bbox__) */
