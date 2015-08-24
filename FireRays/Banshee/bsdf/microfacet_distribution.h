

#ifndef MICRROFACET_DISTRIBUTION_H
#define MICRROFACET_DISTRIBUTION_H

///< Interface for microfacet distribution for Microfacet BRDF
///< Returns the probability that microfacet has orientation w.
///<
class MicrofacetDistribution
{
public:
    // Destructor
    virtual ~MicrofacetDistribution(){}
    // w - microfacet orientation (normal), n - surface normal
    virtual float D(float3 const& w, float3 const& n) const = 0;
    // Sample the direction accordingly to this distribution
    virtual void Sample(ShapeBundle::Hit const& hit, float2 const& sample, float3 const& wi, float3& wo, float& pdf) const = 0;
    // PDF of the given direction
    virtual float GetPdf(ShapeBundle::Hit const& hit, float3 const& wi, float3 const& wo) const = 0;
    // Shadowing function also depends on microfacet distribution
    virtual float G(float3 const& wi, float3 const& wo, float3 const& wh, float3 const& n ) const = 0;
};

///< Blinn distribution of microfacets based on Gaussian distribution : D(wh) ~ (dot(wh, n)^e)
///< D(wh) = (e + 2) / (2*PI) * dot(wh,n)^e
///<
class BlinnDistribution: public MicrofacetDistribution
{
public:
    // Constructor
    BlinnDistribution(float e)
    : e_(e)
    {
    }
    
    // Distribution fucntiom
    float D(float3 const& w, float3 const& n) const
    {
        float ndotw = fabs(dot(n, w));
        return (1.f / (2*PI)) * (e_ + 2) * pow(ndotw, e_);
    }
    
    // Sample the distribution
    void Sample(ShapeBundle::Hit const& hit, float2 const& sample, float3 const& wi, float3& wo, float& pdf) const
    {
        // Sample halfway vector first, then reflect wi around that
        float costheta = std::pow(sample.x, 1.f / (e_ + 1.f));
        float sintheta = std::sqrt(1.f - costheta * costheta);
        
        // phi = 2*PI*ksi2
        float cosphi = std::cos(2.f*PI*sample.y);
        float sinphi = std::sin(2.f*PI*sample.y);
        
        // Calculate wh
        float3 wh = normalize(hit.dpdu * sintheta * cosphi + hit.dpdv * sintheta * sinphi + hit.n * costheta);
        
        // Reflect wi around wh
        wo = -wi + 2.f*dot(wi, wh) * wh;
        
        // Calc pdf
        pdf = GetPdf(hit, wi, wo);
    }
    
    // PDF of the given direction
    float GetPdf(ShapeBundle::Hit const& hit, float3 const& wi, float3 const& wo) const
    {
        // We need to convert pdf(wh)->pdf(wo)
        float3 wh = normalize(wi + wo);
        // costheta
        float ndotwh = dot(hit.n, wh);
        // See Humphreys and Pharr for derivation
        return ((e_ + 1.f) * std::pow(ndotwh, e_)) / (2.f * PI * 4.f * dot (wo,wh));
    }
    
    // Shadowing function also depends on microfacet distribution
    float G(float3 const& wi, float3 const& wo, float3 const& wh, float3 const& n ) const
    {
        float ndotwh = fabs(dot(n, wh));
        float ndotwo = fabs(dot(n, wo));
        float ndotwi = fabs(dot(n, wi));
        float wodotwh = fabs(dot(wo, wh));
        
        return std::min(1.f, std::min(2.f * ndotwh * ndotwo / wodotwh, 2.f * ndotwh * ndotwi / wodotwh));
    }
    
    // Exponent
    float e_;
};


///< Backmann distribution
///< Microfacet Models for Refraction through Rough Surfaces
///< Bruce Walter1† Stephen R. Marschner1 Hongsong Li1,2 Kenneth E. Torrance1
///<
class BeckmannDistribution: public MicrofacetDistribution
{
public:
    // Constructor
    BeckmannDistribution(float a)
    : a_(a)
    {
    }
    
    // Distribution fucntion
    float D(float3 const& m, float3 const& n) const
    {
        float ndotm = dot(m, n);
        
        if (ndotm <= 0.f)
            return 0.f;
        
        float ndotm2 = ndotm * ndotm;
        float sinmn = std::sqrt(1.f - clamp(ndotm * ndotm, 0.f, 1.f));
        float tanmn = sinmn / ndotm;
        float a2 = a_ * a_;
        
        float val = (1.f / (PI * a2 * ndotm2 * ndotm2)) * std::exp(-tanmn * tanmn / a2);
        assert(!isnan(val));
        return val;
    }
    
    // Sample the distribution
    void Sample(ShapeBundle::Hit const& hit, float2 const& sample, float3 const& wi, float3& wo, float& pdf) const
    {
        // Sample halfway vector first, then reflect wi around that
        float temp = std::atan(std::sqrt(-a_*a_*std::log(1.f - sample.x)));
        float theta = (float)((temp >= 0)?temp:(temp + 2*PI));
        
        float costheta = std::cos(theta);
        float sintheta = std::sqrt(1.f - clamp(costheta * costheta, 0.f, 1.f));
        
        // phi = 2*PI*ksi2
        float cosphi = std::cos(2.f*PI*sample.y);
        float sinphi = std::sin(2.f*PI*sample.y);
        
        // Calculate wh
        float3 wh = normalize(hit.dpdu * sintheta * cosphi + hit.dpdv * sintheta * sinphi + hit.n * costheta);
        
        // Reflect wi around wh
        wo = -wi + 2.f*dot(wi, wh) * wh;
        
        // Calc pdf
        pdf = GetPdf(hit, wi, wo);
    }
    
    // PDF of the given direction
    float GetPdf(ShapeBundle::Hit const& hit, float3 const& wi, float3 const& wo) const
    {
        // We need to convert pdf(wh)->pdf(wo)
        float3 m = normalize(wi + wo);
        //
        float mpdf = D(m, hit.n) * std::abs(dot(hit.n, m));
        // See Humphreys and Pharr for derivation
        //assert(!isnan(mpdf));
        return mpdf / (4.f * dot (wo, m));
    }
    
    float G1(float3 const& v, float3 const& m, float3 const& n) const
    {
        float ndotv = dot(n, v);
        float mdotv = dot(m, v);
        
        if (ndotv * mdotv <= 0.f)
            return 0.f;
        
        float sinnv = std::sqrt(1.f - clamp(ndotv * ndotv, 0.f, 1.f));
        float tannv = sinnv / ndotv;
        float a = tannv > 0.f ? 1.f / (a_ * tannv) : 0.f;
        float a2 = a * a;
        
        if (mdotv * ndotv <= 0 || a < 1.6f)
            return 1.f;
        
        float val = (3.535f * a + 2.181f * a2) / (1.f + 2.276f * a + 2.577f * a2);
        
        assert(!is_nan(val));
        
        return val;
    }
    
    // Shadowing function also depends on microfacet distribution
    float G(float3 const& wi, float3 const& wo, float3 const& wh, float3 const& n ) const
    {
        return G1(wi, wh, n) * G1(wo, wh, n);
    }
    
    // Parameter
    float a_;
};


///< GGX distrubition, similar to Beckmann, but with stronger tails and shadows
///< Microfacet Models for Refraction through Rough Surfaces
///< Bruce Walter1† Stephen R. Marschner1 Hongsong Li1,2 Kenneth E. Torrance1
///<
class GgxDistribution: public MicrofacetDistribution
{
public:
    // Constructor
    GgxDistribution(float a)
    : a_(a)
    {
    }
    
    // Distribution fucntion
    float D(float3 const& m, float3 const& n) const
    {
        float ndotm = dot(m, n);
        
        if (ndotm <= 0.f)
            return 0.f;
        
        float ndotm2 = ndotm * ndotm;
        float sinmn = std::sqrt(1.f - clamp(ndotm * ndotm, 0.f, 1.f));
        float tanmn = sinmn / ndotm;
        float a2 = a_ * a_;
        
        float val = (a2 / (PI * ndotm2 * ndotm2 * (a2 + tanmn * tanmn) * (a2 + tanmn * tanmn)));
        assert(!isnan(val));
        return val;
    }
    
    // Sample the distribution
    void Sample(ShapeBundle::Hit const& hit, float2 const& sample, float3 const& wi, float3& wo, float& pdf) const
    {
        // Sample halfway vector first, then reflect wi around that
        float temp = std::atan(a_ * std::sqrt(sample.x) / std::sqrt(1.f - sample.x));
        float theta = (float)((temp >= 0)?temp:(temp + 2*PI));
        
        float costheta = std::cos(theta);
        float sintheta = std::sqrt(1.f - clamp(costheta * costheta, 0.f, 1.f));
        
        // phi = 2*PI*ksi2
        float cosphi = std::cos(2.f*PI*sample.y);
        float sinphi = std::sin(2.f*PI*sample.y);
        
        // Calculate wh
        float3 wh = normalize(hit.dpdu * sintheta * cosphi + hit.dpdv * sintheta * sinphi + hit.n * costheta);
        
        // Reflect wi around wh
        wo = -wi + 2.f*dot(wi, wh) * wh;
        
        // Calc pdf
        pdf = GetPdf(hit, wi, wo);
    }
    
    // PDF of the given direction
    float GetPdf(ShapeBundle::Hit const& hit, float3 const& wi, float3 const& wo) const
    {
        // We need to convert pdf(wh)->pdf(wo)
        float3 m = normalize(wi + wo);
        //
        float mpdf = D(m, hit.n) * std::abs(dot(hit.n, m));
        // See Humphreys and Pharr for derivation
        //assert(!isnan(mpdf));
        return mpdf / (4.f * dot (wo, m));
    }
    
    float G1(float3 const& v, float3 const& m, float3 const& n) const
    {
        float ndotv = dot(n, v);
        float mdotv = dot(m,v);
        
        if (ndotv * mdotv <= 0.f)
            return 0.f;
        
        float sinnv = std::sqrt(1.f - clamp(ndotv * ndotv, 0.f, 1.f));
        float tannv = sinnv / ndotv;
        float a2 = a_ * a_;
        
        float val = 2.f / (1.f + std::sqrt(1.f + a2 * tannv * tannv));
        
        assert(!is_nan(val));
        
        return val;
    }
    
    // Shadowing function also depends on microfacet distribution
    float G(float3 const& wi, float3 const& wo, float3 const& wh, float3 const& n ) const
    {
        return G1(wi, wh, n) * G1(wo, wh, n);
    }
    
    // Parameter
    float a_;
};



#endif
