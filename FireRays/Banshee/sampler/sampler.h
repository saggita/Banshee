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

#ifndef SAMPLER_H
#define SAMPLER_H

#include "../math/float2.h"

///< Sampler class defines and interface to
///< entities capable of providing sample points
///< either in 2D space.
///< Samplers are used by rendering tasks and
///< cloned if needed to be used in paralled 
///< environment.
///< Sampler is generating subsequent "patterns" each consisting of num_samples samples
///< When Sample2D is called num_samples times the sampler goes to the next pattern.
///< For some samplers it is required to Flush the seed (for example low-discrepancy Sobol)
///< to start the new sequence when moving from one pixel to another.
///<
class Sampler
{
public:
    // Destructor
    virtual ~Sampler(){}

    // Calculate 2D sample in [0..1]x[0..1]
    virtual float2 Sample2D() const = 0;

    // Returns the number of samples in a pattern
    virtual int num_samples() const = 0;

    // Clone an instance of a sampler
    virtual Sampler* Clone() const = 0;

	// Reset the sequence
	virtual void Reset() {}
};



#endif // SAMPLER_H