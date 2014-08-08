#ifndef MCRNG_H
#define MCRNG_H

#include "rng.h"

///< Marsaglia multiply-with-carry psuedo random number generator.  It's very fast
///< and has good distribution properties.  Has a period of 2^60. See
///< http://groups.google.com/group/sci.crypt/browse_thread/thread/ca8682a4658a124d/
///<
class McRng : public Rng
{
public:
    McRng(unsigned z = 362436069, unsigned w = 521288629)
        : z_(z), w_(w) 
    { }

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