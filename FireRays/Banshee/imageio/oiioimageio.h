#ifndef OIIOIMAGEIO_H
#define OIIOIMAGEIO_H

#include "imageio.h"

class OiioImageIo : public ImageIo
{
public:

    void Read(std::string const& name, std::vector<float>& data, ImageDesc& desc);
    void Write(std::string const& name, std::vector<float> const& data, ImageDesc const& desc);
};



#endif