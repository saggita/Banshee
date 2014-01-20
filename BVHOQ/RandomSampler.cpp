//
//  RandomSampler.cpp
//  BVHOQ
//
//  Created by dmitryk on 03.01.14.
//  Copyright (c) 2014 Dmitry Kozlov. All rights reserved.
//

#include "RandomSampler.h"

#include "utils.h"

RandomSampler::RandomSampler(unsigned numPatterns, unsigned numSamples)
: SamplerBase(numPatterns, numSamples)
{
    
}

void RandomSampler::GenerateSamples()
{
    for (int i = 0; i < GetNumPatterns(); ++i)
        for (int j = 0; j < GetNumSamples(); ++j)
        {
            SetSample(i, j, rand_float());
        }
}
