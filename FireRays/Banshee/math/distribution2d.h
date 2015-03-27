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
#ifndef DISTRIBUTION2D_H
#define DISTRIBUTION2D_H

#include <memory>

#include "float2.h"
#include "distribution1d.h"

///< The class represents 2D piecewise constant distribution of random variable.
///< The PDF is proprtional to passed function defined at NxM points in [0,1]x[0,1] interval
///<
class Distribution2D
{
public:
    // values are function values at equal spacing at nxm points within [0,1]x[0,1] range
    Distribution2D(int n, int m,  float const* values);
    
    // Sample one value using this distribution
    float2 Sample2D(float2 u, float& pdf) const;
    
    // PDF
    float Pdf(float2 uv);
    
private:
    // Dimension of the grid
    int n_, m_;
    // 1D conditional distributions for rows (m_ lines)
    std::vector<std::unique_ptr<Distribution1D> > conddist_;
    // Marginal density
    std::unique_ptr<Distribution1D> marginaldist_;
};


#endif // DISTRIBUTION1D_H
