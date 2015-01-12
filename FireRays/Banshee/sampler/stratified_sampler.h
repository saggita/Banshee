#ifndef STRATIFIED_SAMPLER_H
#define STRATIFIED_SAMPLER_H

#include <memory>
#include <vector>
#include <numeric>


#include "../rng/rng.h"
#include "sampler.h"

///< This is really simple streaming stratified sampler.
///< It doesn't precompute any sampling patterns and 
///< calculates each new one on-the-fly.
///< It relies on RNG instance to provide numbers with decent speed.
///<
class StratifiedSampler : public Sampler
{
public:
    StratifiedSampler(int gridsize, Rng* rng)
        : rng_(rng)
        , gridsize_(gridsize)
        , sampleidx_(0)
        , cellsize_(1.f / gridsize)
        , permutation_(gridsize*gridsize)
    {
        std::iota(permutation_.begin(), permutation_.end(), 0);
    }

    // Calculate 2D sample in [0..1]x[0..1]
    float2 Sample2D() const;

    // Returns the number of samples in a pattern
    int num_samples() const
    {
        return gridsize_ * gridsize_;
    }

    // Clone an instance of a sampler
    Sampler* Clone() const
    {
        return new StratifiedSampler(gridsize_, rng_->Clone());
    }

private:
    // RNG to use
    std::unique_ptr<Rng> rng_;
    // Grid size
    int gridsize_;
    // Grid cell size
    float cellsize_;
    // Current sample inded
    mutable int sampleidx_;
    // Samples permutation
    mutable std::vector<int> permutation_;
};

#endif // STRATIFIED_SAMPLER_H