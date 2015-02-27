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

#ifndef SH_H
#define SH_H

#include "float3.h"

///< In mathematics, spherical harmonics are the angular portion of a set of solutions to Laplace's equation. 
///< Represented in a system of spherical coordinates, Laplace's spherical harmonics 
///<  Y_l_m are a specific set of spherical harmonics that forms an orthogonal system, first introduced by Pierre Simon de Laplace in 1782.
///< Spherical harmonics are important in many theoretical and practical applications, 
///< particularly in the computation of atomic orbital electron configurations, 
///< representation of gravitational fields, geoids, and the magnetic fields of planetary bodies and stars, 
///< and characterization of the cosmic microwave background radiation. 
///< In 3D computer graphics, spherical harmonics play a special role in a wide variety of topics including 
///< indirect lighting (ambient occlusion, global illumination, precomputed radiance transfer, etc.) and modelling of 3D shapes.
///<

///< The function gives the total number of coefficients required for the term up to specified band
inline int NumShTerms(int l)
{
    return (l + 1)*(l + 1);
}

///< The function flattens 2D (l,m) index into 1D to be used in array indexing
inline int ShIndex(int l, int m)
{
    return l*l + l + m;
}


///< The function evaluates the value of Y_l_m(p) coefficient up to lmax band. 
///< coeffs array should have at least NumShTerms(lmax) elements.
void ShEvaluate(float3 const&p, int lmax, float* out);


///< Apply convolution with dot(n,wi) term
void ShConvolveCosTheta(int lmax, float3 const* cin, float3* cout);



#endif // SH_H