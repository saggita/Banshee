#include "oiio_texturesystem.h"

OIIO_NAMESPACE_USING

// Translate TextureSystem options to OIIO
static TextureOpt::InterpMode OiioFilter(OiioTextureSystem::Options::Filter filter)
{
    switch (filter)
    {
    case OiioTextureSystem::Options::kPoint:
        return TextureOpt::InterpClosest;
    case OiioTextureSystem::Options::kBilinear:
        return TextureOpt::InterpBilinear;
    }
    return TextureOpt::InterpClosest;
}

static TextureOpt::Wrap OiioWrapMode(OiioTextureSystem::Options::WrapMode wrapmode)
{
    switch (wrapmode)
    {
    case OiioTextureSystem::Options::kRepeat:
        return TextureOpt::WrapPeriodic;
    case OiioTextureSystem::Options::kMirror:
        return TextureOpt::WrapMirror;
    }
    return TextureOpt::WrapPeriodic;
}

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

float3 OiioTextureSystem::Sample(std::string const& filename, float2 const& uv, float2 const& duvdx, Options const& opts) const
{
    ustring name = ustring(filename.c_str());
    TextureOpt options = TextureOpt();
    options.nchannels = 3;
    options.swrap = OiioWrapMode(opts.wrapmode);
    options.twrap = OiioWrapMode(opts.wrapmode);
    options.interpmode = OiioFilter(opts.filter);

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

// Query texture information
void OiioTextureSystem::GetTextureInfo(std::string const& filename, TextureDesc& texdesc) const
{
    ustring name = ustring(filename.c_str());

    ImageSpec const* spec = texturesys_->imagespec(name, 0); 

    if (spec)
    {
        texdesc.width = spec->width;
        texdesc.height = spec->height;
    }
    else
    {
        throw std::runtime_error("OpenImageIO error");
    }
}