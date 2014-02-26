//
//  OCLBVHBackEnd.h
//  FireRays
//
//  Created by dmitryk on 25.02.14.
//
//

#ifndef OCLBVHBACKEND_H
#define OCLBVHBACKEND_H

#include "BVH.h"

class OCLBVHBackEnd
{
public:
    struct Node;
    OCLBVHBackEnd(BVH& bvh);
    
    void Generate();
    
    Node const*  GetNodes() const;
    unsigned int GetNodeCount() const;
    
private:
    OCLBVHBackEnd(OCLBVHBackEnd const&);
    OCLBVHBackEnd& operator = (OCLBVHBackEnd const&);
    
    BVH& bvh_;
    std::vector<Node> nodes_;
};

struct OCLBVHBackEnd::Node
{
    BBox box;
    unsigned primStartIdx;
    unsigned right;
    unsigned parent;
    unsigned primCount;
};



#endif // OCLBVHBACKEND_H
