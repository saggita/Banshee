#include "texturetracer.h"

#include "../world/world.h"
#include "../texture/texturesystem.h"

float3 TextureTracer::GetLi(ray const& r, World const& world, Sampler const& lightsampler, Sampler const& brdfsampler) const
{
    ShapeBundle::Hit hit;
    
    if (world.Intersect(r, hit))
    {
        float3 res = texsys_.Sample(texture_, hit.uv, float2(), {TextureSystem::Options::kPoint, TextureSystem::Options::kRepeat});
        return float3(res.x, res.x, res.x);
    }
    
    return float3();
}


