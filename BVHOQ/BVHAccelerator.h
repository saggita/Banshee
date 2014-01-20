//
//  BVHAccelerator.h
//  BVHOQ
//
//  Created by dmitryk on 09.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#ifndef __BVHOQ__BVHAccelerator__
#define __BVHOQ__BVHAccelerator__

#include "CommonTypes.h"
#include "SceneBase.h"
#include "BBox.h"

#include <list>
#include <memory>

class BVHAccelerator
{
public:
	static std::shared_ptr<BVHAccelerator> CreateFromScene(SceneBase const& scene);
	
	template <typename T> BVHAccelerator(std::vector<T> const& vertices, std::vector<unsigned> const& indices, std::vector<unsigned> const& materials, unsigned max_node_prims);
	~BVHAccelerator();
	
	struct Node
	{
		BBox box;
		unsigned primStartIdx;
		unsigned right;
		unsigned parent;
		unsigned primCount;
	};
	
	struct Triangle
	{
		unsigned i1,i2,i3,m;
	};
	
	//std::vector<vector3> const& GetVertices() const;
	std::vector<Triangle> const& GetPrimitives() const;
	std::vector<Node> const& GetNodes() const;
	int GetRootNode() const;
	
	//bool Intersect(ray const& r, float& t);
	
private:
	struct BuildNode
	{
		BBox box;
		unsigned primStartIdx;
		unsigned left;
		unsigned right;
		unsigned parent;
		unsigned primCount;
	};

	struct BuildInfo
	{
		BBox box;
		unsigned primIdx;
	};

	unsigned CreateLeafNode(std::vector<BuildNode>& buildNodes, unsigned begin, unsigned end, BBox const& b);
	unsigned FindBestSplit(unsigned begin, unsigned end, BBox const& box, BBox const& centroidBounds, float& sahValue);
	unsigned BuildHierarchy(unsigned begin, unsigned end, std::vector<BuildNode>& buildNodes);
	unsigned Linearize(unsigned build_node_idx, unsigned parent, std::vector<BuildNode> const& buildNodes);
	bool     Intersect(int idx, ray const& r, float& t);
	
	template <typename T> BBox CalcBbox(Triangle const& t, std::vector<T> const& vertices);
	
	// temporary solution to keep geometry here
	//std::vector<vector3> vertices_;
	std::vector<Triangle> primsReordered_;
	std::vector<Triangle> prims_;
	std::vector<BuildInfo> buildInfo_;
	std::vector<Node> nodes_;
	
	unsigned maxNodePrims_;
	int root_;
};



#endif /* defined(__BVHOQ__BVHAccelerator__) */
