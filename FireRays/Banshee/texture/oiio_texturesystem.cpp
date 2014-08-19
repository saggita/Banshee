#include "oiio_texturesystem.h"

OIIO_NAMESPACE_USING

OiioTextureSystem::OiioTextureSystem(std::string const& searchpath)
    : texturesys_(OIIO_NAMESPACE::TextureSystem::create())
{
    // Set search path to OIIO
    const char *path = searchpath.c_str();
    texturesys_->attribute ("searchpath", TypeDesc::STRING, &path);
}

OiioTextureSystem::~OiioTextureSystem()
{
    OIIO_NAMESPACE::TextureSystem::destroy(texturesys_);
}

float3 OiioTextureSystem::Sample(std::string const& filename, float2 const& uv, float2 const& duvdx) const
{
    ustring name = ustring(filename.c_str());
    TextureOpt options = TextureOpt();
    options.nchannels = 3;
    options.swrap = TextureOpt::WrapPeriodic;
    options.twrap = TextureOpt::WrapPeriodic;

    float3 result;

    // TODO: add support for dudv/dxdy
    if (texturesys_->texture(name, options, uv.x, uv.y, 0.f, 0.f, 0.f, 0.f, &result.x))
    {
        return result;
    }
    else
    {
        throw std::runtime_error("Texture fetch failed");
    }
}