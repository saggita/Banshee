//
//  BVHBuilderBase.h
//  FireRays
//
//  Created by dmitryk on 21.02.14.
//
//

#ifndef BVHBUILDERBASE_H
#define BVHBUILDERBASE_H

#include "BVH.h"

class BVHBuilderBase
{
public:
    BVHBuilderBase() : bvh_ (nullptr) {}
    virtual ~BVHBuilderBase(){}
    virtual void SetBVH(BVH* bvh){ bvh_ = bvh; }
    virtual void Build() = 0;
    
protected:
    BVH& GetBVH() const { return *bvh_; }
    BVH* bvh_;
};

#endif // BVHBUILDERBASE_H
