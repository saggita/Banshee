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
#ifndef FRESNEL_H
#define FRESNEL_H

///< The class defines an interface for different Fresnel light distributions between reflected and transmitted component:
///< - Dielectrics:
///< - Conductors:
///<
class Fresnel
{
public:
    // Destructor
    virtual ~Fresnel(){}
    // Evaluate Fresnel component, etai - incident ray medium IOR, etat - trasmissive ray medium IOR,
    // ndotwi - zenith angle of incident direction
    virtual float Evaluate(float etai, float etat, float ndotwi) = 0;
};


///< Fresnel equations for dielectrics
///<
class FresnelDielectric : public Fresnel
{
public:
    
    // Evaluate Fresnel component, etai - incident ray medium IOR, etat - trasmissive ray medium IOR,
    // ndotwi - zenith angle of incident direction
    float Evaluate(float etai, float etat, float ndotwi)
    {
        float ei = etai;
        float et = etat;
        
        // Swap IORs in case of a back face
        if (ndotwi < 0)
        {
            ei = etat;
            et = etai;
            ndotwi = -ndotwi;
        }
        
        // Evaluate sin(theta_t)
        float sint = (ei / et) * sqrtf(std::max(0.f, 1.f - ndotwi * ndotwi));
        
        if (sint >= 1.f)
        {
            // Total internal reflection
            return 1.f;
        }
        else
        {
            // cos(theta_t)
            float ndotwt = sqrtf(std::max(0.f, 1.f - sint * sint));
            
            // Parallel and perpendicular polarization
            float rparl = ((et * ndotwi) - (ei * ndotwt)) / ((et * ndotwi) + (ei * ndotwt));
            float rperp = ((ei * ndotwi) - (et * ndotwt)) / ((ei * ndotwi) + (et * ndotwt));
            return (rparl*rparl + rperp*rperp) * 0.5f;
        }
    }
};



#endif
