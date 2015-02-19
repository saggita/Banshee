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

#ifndef MCRNG_H
#define MCRNG_H

#include "../math/mathutils.h"

#include "rng.h"

///< Marsaglia multiply-with-carry psuedo random number generator.  It's very fast
///< and has good distribution properties.  Has a period of 2^60. See
///< http://groups.google.com/group/sci.crypt/browse_thread/thread/ca8682a4658a124d/
///<
class McRng : public Rng
{
public:
    McRng()
        : z_(rand_uint())
        , w_(rand_uint()) 
    { 
    }

    float NextFloat() const
    {
        return  NextUint() * 2.328306e-10f;
    }


    unsigned NextUint() const
    {
        z_ = 36969 * (z_ & 65535) + (z_ >> 16);
        w_ = 18000 * (w_ & 65535) + (w_ >> 16);
        return (z_ << 16) + w_;
    }

    Rng* Clone() const
    {
        return new McRng();
    }


private:
    mutable unsigned z_;
    mutable unsigned w_;
};

#endif