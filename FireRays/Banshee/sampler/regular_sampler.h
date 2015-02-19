/*
    Banshee and all code, documentation, and other materials contained
    therein are:

        Copyright 2013 Dmitry Kozlov
        All Rights Reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:
        * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
        * Neither the name of the software's owners nor the names of its
        contributors may be used to endorse or promote products derived from
        this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
    (This is the Modified BSD License)
*/

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