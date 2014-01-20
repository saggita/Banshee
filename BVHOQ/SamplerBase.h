//
//  SamplerBase.h
//  BVHOQ
//
//  Created by dmitryk on 03.01.14.
//  Copyright (c) 2014 Dmitry Kozlov. All rights reserved.
//

#ifndef __BVHOQ__SamplerBase__
#define __BVHOQ__SamplerBase__

#include "CommonTypes.h"
#include <vector>

class SamplerBase
{
public:
    SamplerBase(unsigned numPatterns, unsigned numSamples);
    virtual ~SamplerBase();
    
    vector2  GetSample(unsigned pattern, unsigned sample);
    unsigned GetNumPatterns();
    unsigned GetNumSamples();
    
protected:
    void SetSample(unsigned pattern, unsigned sample, vector2 val);
    
    virtual void GenerateSamples() = 0;
    
private:
    std::vector<vector2> samples_;
    unsigned numSamples_;
};

#endif /* defined(__BVHOQ__SamplerBase__) */
