#include "ditracer.h"

#include "../math/mathutils.h"
#include "../world/world.h"
#include "../light/light.h"
#include "../material/material.h"
#include "../sampler/sampler.h"
#include "../bsdf/bsdf.h"

#include <algorithm>
#include <functional>
#include <cassert>
#include <iostream>

#include "../math/mathutils.h"

#define MINPDF 0.05f

float3 DiTracer::GetLi(ray const& r, World const& world, Sampler const& lightsampler, Sampler const& brdfsampler) const
{
    ShapeBundle::Hit hit;
    float3 radiance;

    if (world.Intersect(r, hit))
    {
        // If we hit emissive object just return its emission characteristic
        Material const& mat = *world.materials_[hit.m];

        if (mat.IsEmissive())
        {
            return 0.f;
        }
        else
        {
            for (int i = 0; i < world.lights_.size(); ++i)
            {
                // TODO: Pass RNG or sampler
                //int numlights = world.lights_.size();
                //int idx = rand_uint() % numlights;
                radiance += GetDi(world, *world.lights_[i], lightsampler, brdfsampler, -r.d, hit);
            }
        }
    }
    else
    {
        radiance = world.bgcolor_;

        for (int i = 0; i < world.lights_.size(); ++i)
        {
            radiance += world.lights_[i]->GetLe(r);
        }
    }

    return radiance;
}

float3 DiTracer::GetDi(World const& world, Light const& light, Sampler const& lightsampler, Sampler const& bsdfsampler, float3 const& wo, ShapeBundle::Hit& hit) const
{
    float3 radiance;
    // TODO: fix that later with correct heuristic
    assert(lightsampler.num_samples() == bsdfsampler.num_samples());
    
    // Sample light source first to apply MIS later
    {
        // Direction from the shading point to the light
        float3 lightdir;
        // PDF for BSDF sample
        float bsdfpdf = 0.f;
        // PDF for light sample
        float lightpdf = 0.f;
        // Sample numsamples times
        int numsamples = lightsampler.num_samples();
        // Allocate samples
        std::vector<float2> lightsamples(numsamples);
        std::vector<float2> bsdfsamples(numsamples);

        // Generate samples
        for (int i = 0; i < numsamples; ++i)
        {
            lightsamples[i] = lightsampler.Sample2D();
            bsdfsamples[i] = bsdfsampler.Sample2D();
        }
        
        // Cache singularity flag to avoid virtual call in the loop below
        bool singularlight = light.Singular();
        
        // Fetch the material
        Material const& mat = *world.materials_[hit.m];

        // Start sampling
        for (int i=0; i<numsamples; ++i)
        {
            lightpdf = 0.f;
            bsdfpdf = 0.f;
            
            // This is needed to support normal mapping.
            // Original intersection needs to be kept around since bsdf might alter the normal
            ShapeBundle::Hit hitlocal = hit;
            // Sample light source
            float3 le = light.GetSample(hitlocal, lightsamples[i], lightdir, lightpdf);
            
            // Continue if intensity > 0 and there is non-zero probability of sampling the point
            if (lightpdf > MINPDF && le.sqnorm() > 0.f)
            {
                // Normalize direction to light
                float3 wi = normalize(lightdir);
                // Calculate distance for shadow testing
                float  dist = sqrtf(lightdir.sqnorm());
                
                // Spawn shadow ray
                ray shadowray;
                // From an intersection point
                shadowray.o = hitlocal.p;
                // Into evaluated direction
                shadowray.d = wi;
                
                // TODO: move ray epsilon into some global options object
                shadowray.t = float2(0.01f, dist - 0.01f);
                
                // Check for an occlusion
                float shadow = world.Intersect(shadowray) ? 0.f : 1.f;
                
                // If we are not in shadow
                if (shadow > 0.f)
                {
                    // Evaluate BSDF
                    float3 bsdf = mat.Evaluate(hitlocal, wi, wo);
                    
                    // We can't apply MIS for singular lights, so use simple estimator
                    if (singularlight)
                    {
                        // Estimate with Monte-Carlo L(wo) = int{ Ld(wi, wo) * fabs(dot(n, wi)) * dwi }
                        radiance +=  le * bsdf * fabs(dot(hitlocal.n, wi)) * (1.f / lightpdf);
                        assert(!has_nans(radiance));
                    }
                    else
                    {
                        // Apply MIS
                        bsdfpdf = mat.GetPdf(hitlocal, wi, wo);
                        // Evaluate weight
                        float weight = PowerHeuristic(1, lightpdf, 1, bsdfpdf);
                        // Estimate with Monte-Carlo L(wo) = int{ Ld(wi, wo) * fabs(dot(n, wi)) * dwi }
                        radiance +=  le * bsdf * fabs(dot(hitlocal.n, wi)) * weight * (1.f / lightpdf);
                        assert(!has_nans(radiance));
                    }
                }
            }

            // Sample BSDF if the light is not singular
            if (!singularlight)
            {
                int bsdftype = 0;
                float3 wi;

                // Sample material
                float3 bsdf = mat.Sample(hitlocal, bsdfsamples[i], wo, wi, bsdfpdf, bsdftype);
                //assert(!has_nans(bsdf));
                //assert(!has_nans(bsdfpdf < 1000000.f));

                // Normalize wi
                wi = normalize(wi);
                
                // If something would be reflected
                if (bsdf.sqnorm() > 0.f && bsdfpdf > MINPDF)
                {
                    float weight = 1.f;
                
                    // Apply MIS if BSDF is not specular
                    if (! (bsdftype & Bsdf::SPECULAR))
                    {
                        // Evaluate light PDF
                        lightpdf = light.GetPdf(hitlocal, wi);

                        // If light PDF is zero skip to next sample
                        if (lightpdf < MINPDF)
                        {
                            continue;
                        }
                        
                        // Apply heuristic
                        weight = PowerHeuristic(1, bsdfpdf, 1, lightpdf);
                    }

                    // Spawn shadow ray
                    ray shadowray;
                    // From an intersection point
                    shadowray.o = hitlocal.p;
                    // Into evaluated direction
                    shadowray.d = wi;

                    // TODO: move ray epsilon into some global options object
                    shadowray.t = float2(0.01f, 10000000.f);

                    // Cast the ray into the scene
                    ShapeBundle::Hit shadowhit;
                    float3 le(0.f, 0.f, 0.f);
                    // If the ray intersects the scene check if we have intersected this light
                    // TODO: move that to area light class
                    if (world.Intersect(shadowray, shadowhit))
                    {
                        // Only sample if this is our light
                        if ((Light const*)shadowhit.bundle->GetAreaLight() == &light)
                        {
                            Material const& lightmat = *world.materials_[shadowhit.m];
                            // Get material emission properties
                            ShapeBundle::Sample sampledata(shadowhit);

                            float3 d = sampledata.p - hitlocal.p;

                            // If the object facing the light compute emission
                            if (dot(sampledata.n, -wi) > 0.f)
                            {
                                // Emissive power with squared fallof
                                float d2inv = 1.f / d.sqnorm();

                                // Return emission characteristic of the material
                                le = lightmat.GetLe(sampledata, -wi) * d2inv;
                            }
                        }
                    }
                    else
                    {
                        // This is to give a chance for IBL to contribute
                        le = light.GetLe(shadowray);
                    }
                    
                    if (le.sqnorm() > 0.f)
                    {
                        // Estimate with Monte-Carlo L(wo) = int{ Ld(wi, wo) * fabs(dot(n, wi)) * dwi }
                        radiance +=  le * bsdf * fabs(dot(hitlocal.n, wi)) * weight * (1.f / bsdfpdf);
                        //assert(!has_nans(radiance));
                    }
                }
            }
        }

        return (1.f / numsamples) * radiance;
    }
}