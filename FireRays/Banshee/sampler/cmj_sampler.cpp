#include "cmj_sampler.h"

static unsigned int permute(unsigned i, unsigned l, unsigned p)
{
    unsigned w = l - 1;
    w |= w >> 1;
    w |= w >> 2;
    w |= w >> 4;
    w |= w >> 8;
    w |= w >> 16;
    
    do
    {
        i ^= p;
        i *= 0xe170893d;
        i ^= p >> 16;
        i ^= (i & w) >> 4;
        i ^= p >> 8;
        i *= 0x0929eb3f;
        i ^= p >> 23;
        i ^= (i & w) >> 1;
        i *= 1 | p >> 27;
        i *= 0x6935fa69;
        i ^= (i & w) >> 11;
        i *= 0x74dcb303;
        i ^= (i & w) >> 2;
        i *= 0x9e501cc3;
        i ^= (i & w) >> 2;
        i *= 0xc860a3df;
        i &= w;
        i ^= i >> 5;
    }
    while (i >= l);
    return (i + p) % l;
}

static float randfloat(unsigned i, unsigned p)
{
    i ^= p;
    i ^= i >> 17;
    i ^= i >> 10;
    i *= 0xb36534e5;
    i ^= i >> 12;
    i ^= i >> 21;
    i *= 0x93fc4795;
    i ^= 0xdf6e307f;
    i ^= i >> 17;
    i *= 1 | p >> 18;
    return i * (1.0f / 4294967808.0f);
}

static float2 cmj(int s, int m, int n, int p)
{
    int sx = permute(s % m, m, p * 0xa511e9b3);
    int sy = permute(s / m, n, p * 0x63d83595);
    float jx = randfloat(s, p * 0xa399d265);
    float jy = randfloat(s, p * 0x711ad6a5);
    
    return float2((s % m + (sy + jx) / n) / m,
                  (s / m + (sx + jy) / m) / n);
}

// Calculate 2D sample in [0..1]x[0..1]
float2 CmjSampler::Sample2D() const
{
    if (sampleidx_ % (gridsize_ * gridsize_) == 0)
    {
        sampleidx_ = 0;
        patternidx_ = rng_->NextUint();
    }
    
    return cmj(sampleidx_++, gridsize_, gridsize_, patternidx_);
}
