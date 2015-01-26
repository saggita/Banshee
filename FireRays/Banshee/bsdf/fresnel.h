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
