//
//  BVH.h
//  BVHOQ
//
//  Created by dmitryk on 09.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#ifndef BVH_H
#define BVH_H

#include "Common.h"
#include "bbox.h"

class BVH
{
public:
    BVH();
    ~BVH();

protected:
    // interface for the builders
    typedef unsigned NodeId;
    struct NodeDesc;

    NodeId GetRootId() const;
    NodeId AttachInternalNode(NodeId id, BBox& b);
    NodeId AttachLeaf(NodeId id, BBox& b, unsigned primStartIdx, unsigned primCount); 
    bool   CheckInvariant();


private:
    BVH(BVH const&);
    BVH& operator = (BVH const&);


    struct Node;
    std::unique_ptr<Node> root_;
};

struct BVH::NodeDesc
{
    BBox box;
    unsigned primStartIdx;
    unsigned primCount;
};

inline BVH::BVH(){}
inline BVH::~BVH(){}


#endif