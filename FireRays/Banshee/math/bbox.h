#ifndef BBOX_H
#define BBOX_H

#include <algorithm>
#include <limits>

#include "common_types.h"

class BBox
{
public:
    BBox();
	BBox(vector3 const& p);
	BBox(vector3 const& mn, vector3 const& mx);
	
	vector3 GetCenter() const;
	vector3 GetExtents() const;
	
	bool  Contains(vector3 const& p) const;
    
	int   GetMaxDim() const;
	float GetSurfaceArea() const;

	vector3 pmin;
	vector3 pmax;
};

inline BBox BBoxUnion(BBox const& box1, BBox const& box2)
{	
	return BBox(vmin(box1.pmin, box2.pmin), vmax(box1.pmax, box2.pmax));
}

inline BBox BBoxIntersection(BBox const& box1, BBox const& box2)
{	
	return BBox(vmax(box1.pmin, box2.pmin), vmin(box1.pmax, box2.pmax));
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
	return box1.Contains(box2.pmin) && box1.Contains(box2.pmax);
}


inline vector3 BBox::GetExtents() const
{
    return pmax - pmin;
}

inline vector3 BBox::GetCenter() const
{
    return 0.5f * (pmax + pmin);
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
    vector3 extents = GetExtents();
    return 2 * (extents.x() * extents.y() + extents.x() * extents.z() + extents.y() * extents.z());
}

inline BBox::BBox()
: pmin(vector3(std::numeric_limits<float>::max(),
std::numeric_limits<float>::max(),
std::numeric_limits<float>::max()))
, pmax(vector3(-std::numeric_limits<float>::max(),
-std::numeric_limits<float>::max(),
-std::numeric_limits<float>::max()))
{

}

inline BBox::BBox(vector3 const& p)
: pmin(p)
, pmax(p)
{

}

inline BBox::BBox(vector3 const& p1, vector3 const& p2)
: pmin(vmin(p1, p2))
, pmax(vmax(p1, p2))
{
}

#endif // BBOX_H
