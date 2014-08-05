#ifndef FILEIMAGEPLANE_H
#define FILEIMAGEPLANE_H

#include <string>
#include <vector>

#include "../imageio/imageio.h"
#include "imageplane.h"

///< File image plane is designed to 
///< collect rendering results and output
///< those into an image file
///<
class FileImagePlane : public ImagePlane
{
public:
    FileImagePlane(std::string filename, int2 res, ImageIo& io)
        : filename_(filename)
        , res_(res)
        , io_(io)
        , imgbuf_(res.x * res.y)
    {
    }

    // This method is called by the renderer prior to adding samples
    void Prepare();

    // This method is called by the renderer after adding all the samples
    void Finalize();

    // Add color contribution to the image plane
    void AddSample(float2 const& sample, float w, float3 value);

    // This is used by the renderer to decide on the number of samples needed
    int2 resolution() const { return res_; }

    // File name to write to
    std::string filename_;
    // Image resolution
    int2 res_;
    // IO object
    ImageIo& io_;
    // Intermediate image buffer
    std::vector<float3> imgbuf_;
};


#endif // FILEIMAGEPLANE_H