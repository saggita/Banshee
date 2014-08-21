#ifndef OIIO_TEXTURESYSTEM_H
#define OIIO_TEXTURESYSTEM_H

#include "texturesystem.h"

#include "OpenImageIO/imageio.h"
#include "OpenImageIO/texture.h"

///< Texture system based on OpenImageIO library
///<
class OiioTextureSystem : public TextureSystem
{
public:
    // Specify a search path for the textures
    OiioTextureSystem(std::string const& searchpath);

    // Destructor
    ~OiioTextureSystem();

    // Filtered texture lookup
    // TODO: add support for float4 lookups
    float3 Sample(std::string const& filename, float2 const& uv, float2 const& duvdx, Options const& opts = Options()) const;

private:
    OIIO_NAMESPACE::TextureSystem* texturesys_;
};

#endif //TEXTURESYSTEM_H