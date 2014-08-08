#ifndef TEXTURESYSTEM_H
#define TEXTURESYSTEM_H

#include <string>

#include "../math/float2.h"
#include "../math/float3.h"

///< This class defines an interface for all entities
///< that require texture lookups in the system.
///<
class TextureSystem
{
public:
    TextureSystem(){}
    // Destructor
    virtual ~TextureSystem(){}

    // Filtered texture lookup
    // TODO: add support for texturing options
    // TODO: add support for float4 lookups
    virtual float3 Sample(std::string const& filename, float2 const& uv, float2 const& duvdx) const = 0;

protected:
    TextureSystem(TextureSystem const&);
    TextureSystem& operator =(TextureSystem const&);
};

#endif //TEXTURESYSTEM_H