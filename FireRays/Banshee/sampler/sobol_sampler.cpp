
#include "sobol_sampler.h"

// Van Der Corput sequence
static float VanDerCorput(unsigned int n, unsigned int scramble)
{
	n = ((n >> 16) | (n << 16));
	n = (((n & 0xff00ff00) >> 8) | ((n & 0x00ff00ff) << 8));
	n = (((n & 0xf0f0f0f0) >> 4) | ((n & 0x0f0f0f0f) << 4));
	n = (((n & 0xcccccccc) >> 2) | ((n & 0x33333333) << 2));
	n = (((n & 0xaaaaaaaa) >> 1) | ((n & 0x55555555) << 1));
	n ^= scramble;
	return ((n >> 8) & 0xffffff) / float(1 << 24);
}

// Sobol low discrepancy sequence
static float Sobol02(unsigned int n, unsigned int scramble)
{
	for (unsigned int v = 1 << 31; n != 0; n >>= 1, v ^= v >> 1)
	{
		if (n & 0x1)
		{
			scramble ^= v;
		}
	}

	return ((scramble >> 8) & 0xffffff) / float(1 << 24);
}

float2 SobolSampler::Sample2D() const
{
	float2 sample;
	sample.x = VanDerCorput(sequenceidx_, scramble0_);
	sample.y = Sobol02(sequenceidx_, scramble1_);

	++sequenceidx_;

	return sample;
}

void SobolSampler::Reset()
{
	// Reset sample indices
	sequenceidx_ = 0;

	// Generate new sequence of Sobol samples
	scramble0_ = rng_->NextUint();
	scramble1_ = rng_->NextUint();
}