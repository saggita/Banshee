//
//  RandomSampler.h
//  BVHOQ
//
//  Created by dmitryk on 03.01.14.
//  Copyright (c) 2014 Dmitry Kozlov. All rights reserved.
//

#ifndef __BVHOQ__RandomSampler__
#define __BVHOQ__RandomSampler__

#include "SamplerBase.h"

class RandomSampler : public SamplerBase
{
public:
    RandomSampler(unsigned numPatterns, unsigned numSamples);

protected:
    void GenerateSamples();
};

#endif /* defined(__BVHOQ__RandomSampler__) */
