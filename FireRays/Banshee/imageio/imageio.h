#ifndef IMAGEIO_H
#define IMAGEIO_H

#include <string>
#include <vector>
#include <memory>

class ImageIo
{
public:
    struct ImageDesc
    {
        unsigned xres, yres;
        unsigned nchannels;

        ImageDesc(unsigned w, unsigned h, unsigned nc)
            : xres(w)
            , yres(h)
            , nchannels(nc)
        {}
    };

    ImageIo(){}
    virtual ~ImageIo(){}
    virtual void Read(std::string const& name,  std::vector<float>& data, ImageDesc& desc) = 0;
    virtual void Write(std::string const& name, std::vector<float> const& data, ImageDesc const& desc) = 0;

protected:
    ImageIo(ImageIo const&);
    ImageIo& operator = (ImageIo const&);
};


#endif // IMAGEIO_H