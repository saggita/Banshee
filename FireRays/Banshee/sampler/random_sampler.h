#ifndef RANDOM_SAMPLER_H
#define RANDOM_SAMPLER_H

#include <memory>


#include "../rng/rng.h"
#include "sampler.h"

///< This is really simple streaming random sampler.
///< It doesn't precompute any sampling patterns and 
///< calculates each new one on-the-fly.
///< It relies on RNG instance to provide numbers with decent speed.
///<
class RandomSampler : public Sampler
{
public:
    RandomSampler(int numsamples, Rng* rng)
        : rng_(rng)
        , numsamples_(numsamples)
    {
    }

    // Calculate 2D sample in [0..1]x[0..1]
    float2 Sample2D() const
    {
        return float2(rng_->NextFloat(), rng_->NextFloat());
    }

    // Returns the number of samples in a pattern
    int num_samples() const
    {
        return numsamples_;
    }

private:
    // RNG to use
    std::unique_ptr<Rng> rng_;
    // Number of samples
    int numsamples_;
};

#endif