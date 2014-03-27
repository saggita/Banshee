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
    struct Primitive;
    
    BVHBuilderBase() : bvh_ (nullptr) {}
    virtual ~BVHBuilderBase(){}
    virtual void SetBVH(BVH* bvh){ bvh_ = bvh; }
    virtual void Build() = 0;
    
protected:
    BVH& GetBVH() const { return *bvh_; }
    BVH* bvh_;
};

struct BVHBuilderBase::Primitive
{
    union
    {
        struct
        {
            unsigned i1,i2,i3,m;
        };
        
        unsigned i[4];
    };
};

#endif // BVHBUILDERBASE_H
