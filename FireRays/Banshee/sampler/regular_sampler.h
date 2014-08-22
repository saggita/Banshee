#ifndef REGULAR_SAMPLER_H
#define REGULAR_SAMPLER_H

#include <memory>
#include <vector>

#include "../rng/rng.h"
#include "sampler.h"

///< This is really simple regular sampler.
///< The samples are generated in regular NxN grid.
///<
class RegularSampler : public Sampler
{
public:
    RegularSampler(int gridsize)
        : gridsize_(gridsize)
        , sampleidx_(0)
    {
        float step = 1.f / gridsize_;
        float step2 = 1.f / (2.f * gridsize_);
        samples_.reserve(gridsize_ * gridsize_);
        for (int x=0; x<gridsize_; ++x)
            for (int y=0; y<gridsize_; ++y)
            {
                float2 sample = float2(x * step + step2, y * step + step2);
                samples_.push_back(sample);
            }
    }

    // Calculate 2D sample in [0..1]x[0..1]
    float2 Sample2D() const
    {
        float2 sample = samples_[sampleidx_ % (gridsize_* gridsize_)];
        ++sampleidx_;
        return sample;
    }

    // Returns the number of samples in a pattern
    int num_samples() const
    {
        return gridsize_ * gridsize_;
    }

    // Clone an instance of a sampler
    Sampler* Clone() const
    {
        return new RegularSampler(gridsize_);
    }

private:
    // Samples to use
    std::vector<float2> samples_;
    // Grid size
    int gridsize_;
    // Current sample index
    mutable int sampleidx_;
};

#endif //REGULAR_SAMPLER_H