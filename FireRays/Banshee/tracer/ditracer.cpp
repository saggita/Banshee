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

float3 DiTracer::Li(ray& r, World const& world, Sampler const& lightsampler, Sampler const& brdfsampler) const
{
    Primitive::Intersection isect;
    float t = r.t.y;
    float3 radiance;

    if (world.Intersect(r, t, isect))
    {
        // If we hit emissive object just return its emission characteristic
        Material const& mat = *world.materials_[isect.m];

        if (mat.emissive())
        {
            return 0.f;
        }
        else
        {
            //for (int i = 0; i < world.lights_.size(); ++i)
            //{
            // TODO: Pass RNG or sampler
            int numlights = world.lights_.size();
            int idx = rand_uint() % numlights;
            radiance = Di(world, *world.lights_[idx], lightsampler, brdfsampler, -r.d, isect) * numlights;
            //}
        }
    }
    else
    {
        radiance = world.bgcolor_;

        for (int i = 0; i < world.lights_.size(); ++i)
        {
            radiance += world.lights_[i]->Le(r);
        }
    }

    return radiance;
}

float3 DiTracer::Di(World const& world, Light const& light, Sampler const& lightsampler, Sampler const& bsdfsampler, float3 const& wo, Primitive::Intersection& isect) const
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
        bool singularlight = light.singular();
        
        // Fetch the material
        Material const& mat = *world.materials_[isect.m];

        // Start sampling
        for (int i=0; i<numsamples; ++i)
        {
            lightpdf = 0.f;
            bsdfpdf = 0.f;
            
            // This is needed to support normal mapping.
            // Original intersection needs to be kept around since bsdf might alter the normal
            Primitive::Intersection isectlocal = isect;
            // Sample light source
            float3 le = light.Sample(isectlocal, lightsamples[i], lightdir, lightpdf);
            
            // Continue if intensity > 0 and there is non-zero probability of sampling the point
            if (lightpdf > 0.0f && le.sqnorm() > 0.f)
            {
                // Normalize direction to light
                float3 wi = normalize(lightdir);
                // Calculate distance for shadow testing
                float  dist = sqrtf(lightdir.sqnorm());
                
                // Spawn shadow ray
                ray shadowray;
                // From an intersection point
                shadowray.o = isectlocal.p;
                // Into evaluated direction
                shadowray.d = wi;
                
                // TODO: move ray epsilon into some global options object
                shadowray.t = float2(0.001f, dist - 0.001f);
                
                // Check for an occlusion
                float shadow = world.Intersect(shadowray) ? 0.f : 1.f;
                
                // If we are not in shadow
                if (shadow > 0.f)
                {
                    // Evaluate BSDF
                    float3 bsdf = mat.Evaluate(isectlocal, wi, wo);
                    
                    // We can't apply MIS for singular lights, so use simple estimator
                    if (singularlight)
                    {
                        // Estimate with Monte-Carlo L(wo) = int{ Ld(wi, wo) * fabs(dot(n, wi)) * dwi }
                        radiance +=  le * bsdf * fabs(dot(isectlocal.n, wi)) * (1.f / lightpdf);
                        assert(!has_nans(radiance));
                    }
                    else
                    {
                        // Apply MIS
                        bsdfpdf = mat.Pdf(isectlocal, wi, wo);
                        // Evaluate weight
                        float weight = PowerHeuristic(1, lightpdf, 1, bsdfpdf);
                        // Estimate with Monte-Carlo L(wo) = int{ Ld(wi, wo) * fabs(dot(n, wi)) * dwi }
                        radiance +=  le * bsdf * fabs(dot(isectlocal.n, wi)) * weight * (1.f / lightpdf);
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
                float3 bsdf = mat.Sample(isectlocal, bsdfsamples[i], wo, wi, bsdfpdf, bsdftype);
                //assert(!has_nans(bsdf));
                //assert(!has_nans(bsdfpdf < 1000000.f));

                // Normalize wi
                wi = normalize(wi);
                
                // If something would be reflected
                if (bsdf.sqnorm() > 0.f && bsdfpdf > 0.f)
                {
                    float weight = 1.f;
                
                    // Apply MIS if BSDF is not specular
                    if (! (bsdftype & Bsdf::SPECULAR))
                    {
                        // Evaluate light PDF
                        lightpdf = light.Pdf(isectlocal, wi);

                        // If light PDF is zero skip to next sample
                        if (lightpdf == 0.f)
                        {
                            continue;
                        }
                        
                        // Apply heuristic
                        weight = PowerHeuristic(1, bsdfpdf, 1, lightpdf);
                    }

                    // Spawn shadow ray
                    ray shadowray;
                    // From an intersection point
                    shadowray.o = isectlocal.p;
                    // Into evaluated direction
                    shadowray.d = wi;

                    // TODO: move ray epsilon into some global options object
                    shadowray.t = float2(0.001f, 10000000.f);

                    // Cast the ray into the scene
                    float t = 0.f;
                    Primitive::Intersection shadowisect;
                    float3 le(0.f, 0.f, 0.f);
                    // If the ray intersects the scene check if we have intersected this light
                    // TODO: move that to area light class
                    if (world.Intersect(shadowray, t, shadowisect))
                    {
                        // Only sample if this is our light
                        if ((Light const*)shadowisect.primitive->arealight_ == &light)
                        {
                            Material const& lightmat = *world.materials_[shadowisect.m];
                            // Get material emission properties
                            Primitive::SampleData sampledata(shadowisect);

                            float3 d = sampledata.p - isectlocal.p;

                            // If the object facing the light compute emission
                            if (dot(sampledata.n, -wi) > 0.f)
                            {
                                // Emissive power with squared fallof
                                float d2inv = 1.f / d.sqnorm();

                                // Return emission characteristic of the material
                                le = lightmat.Le(sampledata, -wi) * d2inv;
                            }
                        }
                    }
                    else
                    {
                        // This is to give a chance for IBL to contribute
                        le = light.Le(shadowray);
                    }
                    
                    if (le.sqnorm() > 0.f)
                    {
                        // Estimate with Monte-Carlo L(wo) = int{ Ld(wi, wo) * fabs(dot(n, wi)) * dwi }
                        radiance +=  le * bsdf * fabs(dot(isectlocal.n, wi)) * weight * (1.f / bsdfpdf);
                        //assert(!has_nans(radiance));
                    }
                }
            }
        }

        return (1.f / numsamples) * radiance;
    }
}