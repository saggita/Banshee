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

#ifndef SOBOL_SAMPLER_H
#define SOBOL_SAMPLER_H

#include <memory>
#include <vector>
#include <numeric>

#include "../rng/rng.h"
#include "../math/mathutils.h"
#include "sampler.h"

///< The implemetation of low-discrepancy Sobol sampler
///< https://en.wikipedia.org/wiki/Sobol_sequence
///<
class SobolSampler : public Sampler
{
public:
	SobolSampler(int numsamples, Rng* rng)
		: rng_(rng)
		, numsamples_((int)upper_power_of_two(numsamples))
		, sequenceidx_(0)
	{
		scramble0_ = rng_->NextUint();
		scramble1_ = rng_->NextUint();
	}

	// Calculate 2D sample in [0..1]x[0..1]
	float2 Sample2D() const;

	// Returns the number of samples in a pattern
	int num_samples() const
	{
		return numsamples_;
	}

	// Clone an instance of a sampler
	Sampler* Clone() const
	{
		return new SobolSampler(numsamples_, rng_->Clone());
	}

	// Reset the sequence
	void Reset();

private:
	// RNG to use
	std::unique_ptr<Rng> rng_;
	// Number of samples to generate
	int numsamples_;
	// Scramble values
	unsigned int scramble0_;
	unsigned int scramble1_;
	// Sequence index
	mutable int sequenceidx_;
};

#endif // SOBOL_SAMPLER_H
