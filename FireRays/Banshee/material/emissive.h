#ifndef EMISSIVE_H
#define EMISSIVE_H

#include "material.h"


///< Emissive material adds illumination characteristic to the object.
///< This material needs special treatment in order to accurately
///< compute the radiance for the scene. Not only we need to add its
///< Le contribution to the ray hitting emissive object, but also
///< use these objects for direct illumination computations.
///<
class Emissive : public Material
{
public:
    Emissive (float3 const& e)
        : e_(e)
    {
    }


    // Sample material and return outgoing ray direction along with combined BSDF value
    float3 Sample(Primitive::Intersection const& isect, float2 const& sample, float3 const& wi, float3& wo, float& pdf, int& type) const
    {
        // Make sure to set PDF to 0, method is not supposed to be called 
        pdf = 0.f;

        // This method is not supposed to be called on emissive, but anyway
        return Le(isect, wi);
    }

    // Evaluate combined BSDF value
    float3 Evaluate(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const 
    {
        // This method is not supposed to be called on emissive, but anyway
        return Le(isect, wi);
    }
    
    // PDF of a given direction sampled from isect.p
    float Pdf(Primitive::Intersection const& isect, float3 const& wi, float3 const& wo) const
    {
        return 0.f;
    }

    // Indicate whether the materials has emission component and will be used for direct light evaluation
    bool emissive() const { return true; }

    // Emission component of the material
    float3 Le(Primitive::SampleData const& sampledata, float3 const& wo) const 
    { 
        // Cosine term
        float  ndotwo = dot(sampledata.n, wo);

        if (ndotwo > 0.f)
        {
            // Scale by cosine term
            return e_ * ndotwo;
        }

        return float3(0,0,0);
    }

private:
    // Emissive power
    float3 e_;

};



#endif // EMISSIVE_H