#include "stratified_sampler.h"

#include "../math/int2.h"


// Calculate 2D sample in [0..1]x[0..1]
float2 StratifiedSampler::Sample2D() const
{
    float2 subsample = float2(rng_->NextFloat(), rng_->NextFloat());

    int idx = (sampleidx_++) % (gridsize_ * gridsize_);
    int2 subsampleidx = int2 (idx % gridsize_, idx / gridsize_);

    return float2( (subsampleidx.x + subsample.x) * cellsize_,
                   (subsampleidx.y + subsample.y) * cellsize_ ); 
}