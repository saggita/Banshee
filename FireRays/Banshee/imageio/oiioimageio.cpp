#include "oiioimageio.h"

#ifndef __linux__
#include "OpenImageIO/imageio.h"
#else
#include <OpenImageIO/imageio.h>
#endif


OIIO_NAMESPACE_USING

void OiioImageIo::Read(std::string const& name, std::vector<float>& data, ImageDesc& desc)
{
    ImageInput* in = ImageInput::open (name);

    if (!in)
    {
        throw std::runtime_error("Can't load image file");
    }

    const ImageSpec &spec = in->spec();

    int xres = spec.width;
    int yres = spec.height;
    int channels = spec.nchannels;

    data.resize(xres*yres*channels);

    in->read_image (TypeDesc::FLOAT, &data[0]);

    in->close ();

    desc.xres = xres;
    desc.yres = yres;
    desc.nchannels = channels;

    delete in;
}

void OiioImageIo::Write(std::string const& name, std::vector<float> const& data, ImageDesc const& desc)
{
    ImageOutput* out = ImageOutput::create(name);

    if (!out)
    {
        throw std::runtime_error("Can't create image file on disk");
    }

    ImageSpec spec(desc.xres, desc.yres, desc.nchannels, TypeDesc::UINT8);

    out->open(name, spec);
    out->write_image(TypeDesc::FLOAT, &data[0]);
    out->close();

    delete out;
}
