#ifndef SAMPLER_H
#define SAMPLER_H

#include "../math/float2.h"

///< Sampler class defines and interface to
///< entities capable of providing sample points
///< either in 1D or 2D space
///<
class Sampler
{
public:
    // Destructor
    virtual ~Sampler(){}

    // Calculate 2D sample in [0..1]x[0..1]
    virtual float2 Sample2D() const = 0;

    // Returns the number of samples in a pattern
    virtual int num_samples() const = 0;
};



#endif // SAMPLER_H