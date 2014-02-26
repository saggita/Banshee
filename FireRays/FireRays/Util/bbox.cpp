//
//  BBox.cpp
//  BVHOQ
//
//  Created by dmitryk on 07.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#include "BBox.h"
#include <limits>

BBox::BBox()
: minPoint_(vector3(std::numeric_limits<float>::max(),
                    std::numeric_limits<float>::max(),
                    std::numeric_limits<float>::max()))
, maxPoint_(vector3(-std::numeric_limits<float>::max(),
                    -std::numeric_limits<float>::max(),
                    -std::numeric_limits<float>::max()))
{
    
}

BBox::BBox(vector3 const& p)
: minPoint_(p)
, maxPoint_(p)
{
	
}

BBox::BBox(vector3 const& p1, vector3 const& p2)
: minPoint_(std::min(p1.x(), p2.x()), std::min(p1.y(), p2.y()), std::min(p1.z(), p2.z()))
, maxPoint_(std::max(p1.x(), p2.x()), std::max(p1.y(), p2.y()), std::max(p1.z(), p2.z()))
{
}

vector3& BBox::GetMaxPoint()
{
	return maxPoint_;
}

vector3& BBox::GetMinPoint()
{
	return minPoint_;
}

vector3 const& BBox::GetMaxPoint() const
{
	return maxPoint_;
}

vector3 const& BBox::GetMinPoint() const
{
	return minPoint_;
}

vector3 BBox::GetExtents() const
{
	return maxPoint_ - minPoint_;
}

vector3 BBox::GetCenter() const
{
	return 0.5f * (maxPoint_ + minPoint_);
}

bool BBox::Contains(vector3 const& p) const
{
	vector3 radius = 0.5f * GetExtents();
	return abs(GetCenter().x() - p.x()) <= radius.x() &&
		   abs(GetCenter().y() - p.y()) <= radius.y() &&
		   abs(GetCenter().z() - p.z()) <= radius.z();
}

int BBox::GetMaxDim() const
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

float BBox::GetSurfaceArea() const
{
	return 2 * (GetExtents().x() * GetExtents().y() + GetExtents().x() * GetExtents().z() + GetExtents().y() * GetExtents().z());
}
