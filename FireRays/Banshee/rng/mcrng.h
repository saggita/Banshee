#ifndef MCRNG_H
#define MCRNG_H

#include "../math/mathutils.h"

#include "rng.h"

///< Marsaglia multiply-with-carry psuedo random number generator.  It's very fast
///< and has good distribution properties.  Has a period of 2^60. See
///< http://groups.google.com/group/sci.crypt/browse_thread/thread/ca8682a4658a124d/
///<
class McRng : public Rng
{
public:
    McRng()
        : z_(rand_uint())
        , w_(rand_uint()) 
    { 
    }

    float NextFloat() const
    {
        return  NextUint() * 2.328306e-10f;
    }


    unsigned NextUint() const
    {
        z_ = 36969 * (z_ & 65535) + (z_ >> 16);
        w_ = 18000 * (w_ & 65535) + (w_ >> 16);
        return (z_ << 16) + w_;
    }

    Rng* Clone() const
    {
        return new McRng();
    }


private:
    mutable unsigned z_;
    mutable unsigned w_;
};

#endif