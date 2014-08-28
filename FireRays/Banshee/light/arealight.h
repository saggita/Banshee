#ifndef AREALIGHT_H
#define AREALIGHT_H

class Primitive;
class Material;

#include "light.h"

///< The class represents the light formed by a primitive
///< with an emissive material assigned to it.
///<
class AreaLight: public Light
{
public:
    // The callers responsibility to pass only samplable primitives(i.e. area > 0) in there and only emissive materials.
    // Area light rely on primitives ability to provide sample points on its surface.
    //
    AreaLight(Primitive const& primitive, Material const& material);

    // Sample method generates a direction to the light(d), calculates pdf in that direction (pdf)
    // and returns radiance emitted(return value) into the direction specified by isect
    // Note that no shadow testing occurs here, the method knows nothing about world's geometry
    // and it is renderers responsibility to account for visibility term
    float3 Sample(Primitive::Intersection const& isect, float2 const& sample, float3& d, float& pdf) const;

    // This method is supposed to be called by the renderer when the ray misses the geometry.
    // It allows implementing IBL, etc.
    float3 Le(ray const& r) const
    {
        // Nothing should be emitted here for point light
        return float3(0, 0, 0);
    }
    
private:
    // Primitive declaring the shape of the light
    Primitive const& primitive_;
    // Material
    Material const& material_;
};

#endif // AREALIGHT_H
