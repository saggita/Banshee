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
    // Texturing options
    struct Options
    {
        enum Filter
        {
            kPoint,
            kBilinear
        };

        enum WrapMode
        {
            kRepeat,
            kMirror
        };

        // Texture filter to use
        Filter filter;
        // Address mode to use
        WrapMode wrapmode;
        // Pass filtering mode and address mode
        Options (Filter f = kBilinear, WrapMode w = kRepeat)
            : filter(f)
            , wrapmode(w)
        {
        }
    };

    TextureSystem(){}
    // Destructor
    virtual ~TextureSystem(){}

    // Filtered texture lookup
    // TODO: add support for texturing options
    // TODO: add support for float4 lookups
    virtual float3 Sample(std::string const& filename, float2 const& uv, float2 const& duvdx, Options const& opts = Options()) const = 0;

protected:
    TextureSystem(TextureSystem const&);
    TextureSystem& operator =(TextureSystem const&);
};



#endif //TEXTURESYSTEM_H