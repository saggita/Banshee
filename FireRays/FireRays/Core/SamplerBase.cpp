//
//  SamplerBase.cpp
//  BVHOQ
//
//  Created by dmitryk on 03.01.14.
//  Copyright (c) 2014 Dmitry Kozlov. All rights reserved.
//

#include "SamplerBase.h"

SamplerBase::SamplerBase(unsigned numPatterns, unsigned numSamples)
: numSamples_(numSamples)
, samples_(numPatterns * numSamples)
{
}

SamplerBase::~SamplerBase()
{
    
}

vector2  SamplerBase::GetSample(unsigned pattern, unsigned sample)
{
    assert(sample < numSamples_);
    assert(pattern * numSamples_ + sample < samples_.size());
    
    return samples_[pattern * numSamples_ + sample];
}

unsigned SamplerBase::GetNumPatterns()
{
    return static_cast<unsigned>(samples_.size() / numSamples_);
}

unsigned SamplerBase::GetNumSamples()
{
    return numSamples_;
}

void SamplerBase::SetSample(unsigned pattern, unsigned sample, vector2 val)
{
    assert(sample < numSamples_);
    assert(pattern * numSamples_ + sample < samples_.size());
    samples_[pattern * numSamples_ + sample] = val;
}