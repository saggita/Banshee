#include "environment_light_is.h"

#include "../texture/texturesystem.h"
#include "../math/mathutils.h"
#include "../math/distribution2d.h"


#include <cassert>

static int kDistWidth = 512;
static int kDistHeight = 256;

EnvironmentLightIs::EnvironmentLightIs(TextureSystem const& texsys,
                   std::string const& texture,
                   float scale)
: texsys_(texsys)
, texture_(texture)
, scale_(scale)
{
    // Prepare values for distribution generation
    std::vector<float> img(kDistWidth*kDistHeight);
    
    for (int i=0; i<kDistHeight; ++i)
    {
        for (int j=0; j<kDistWidth; ++j)
        {
            float2 uv((float)j/kDistWidth, (float)i/kDistHeight);
            float sintheta = sinf(PI * (float)(i + 0.5f)/kDistHeight);
            
            img[i*kDistWidth + j] = texsys_.Sample(texture_, uv, float2(0,0)).sqnorm() * sintheta;
        }
    }
    
    radiancedist_.reset(new Distribution2D(kDistWidth, kDistHeight, &img[0]));
}

float3 EnvironmentLightIs::Sample(Primitive::Intersection const& isect, float2 const& sample, float3& d, float& pdf) const
{
    // Sample according to radiance distribution
    float dpdf = 0.f;
    float2 uv = radiancedist_->Sample2D(sample, dpdf);
    
    float theta = uv.y * PI;
    float phi = uv.x * 2.f * PI;
    
    float costheta = cosf(theta), sintheta = sinf(theta);
    float sinphi = sinf(phi), cosphi = cosf(phi);
    
    d = float3(sintheta * cosphi, costheta, sintheta * sinphi);
    
    // Convert PDF to spherical mapping
    pdf = (sintheta == 0.f) ? 0.f : (dpdf / (2.f * PI * PI * sintheta));
    
    // Fetch radiance value and scale it
    return scale_ * texsys_.Sample(texture_, uv, float2(0,0));
}


float3 EnvironmentLightIs::Le(ray const& r) const
{
    // Convert world d coordinates to spherical representation
    float rr, phi, theta;
    cartesian_to_spherical(r.d, rr, phi, theta);
    
    // Compose the textcoord to fetch
    float2 uv(phi / (2*PI), theta / PI);
    
    // Fetch radiance value and scale it
    return scale_ * texsys_.Sample(texture_, uv, float2(0,0));
}

// PDF of a given direction sampled from isect.p
float EnvironmentLightIs::Pdf(Primitive::Intersection const& isect, float3 const& w) const
{
    // Convert world d coordinates to spherical representation
    float rr, phi, theta;
    cartesian_to_spherical(w, rr, phi, theta);
    
    // Get sin(theta)
    float sintheta = sinf(theta);
    
    // Construct sample
    float2 uv = float2(phi / (2*M_PI), theta / (M_PI));
    
    // Get PDF and convert to spherical
    return (sintheta == 0.f) ? 0.f : (radiancedist_->Pdf(uv) / (2.f * M_PI * M_PI * sintheta));
}