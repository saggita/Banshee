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
    
	int GetMaxDim() const;
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

#endif /* defined(__BVHOQ__bbox__) */
