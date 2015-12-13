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

#ifndef CMJ_SAMPLER_H
#define CMJ_SAMPLER_H


#include <memory>
#include <vector>
#include <numeric>


#include "../rng/rng.h"
#include "sampler.h"

///< The implemetation of Pixar's streaming corellated multi-jittered sampler
///< http://graphics.pixar.com/library/MultiJitteredSampling/paper.pdf
///<

class CmjSampler : public Sampler
{
public:
    CmjSampler(int gridsize, Rng* rng)
    : rng_(rng)
    , gridsize_(gridsize)
    , sampleidx_(0)
    , patternidx_(0)
    {
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
        return new CmjSampler(gridsize_, rng_->Clone());
    }
    
private:
    // RNG to use
    std::unique_ptr<Rng> rng_;
    // Grid size
    int gridsize_;
    // Current sample inded
    mutable int sampleidx_;
    // Current pattern index
    mutable int patternidx_;
};




#endif // CMJ_SAMPLER_H
