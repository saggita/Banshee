#include "fileimageplane.h"

#include "../math/mathutils.h"
#include <cmath>


void FileImagePlane::Prepare()
{
}

void FileImagePlane::Finalize()
{
    std::vector<float> plaindata(res_.x * res_.y * 3);

    for (int i = 0; i < res_.x * res_.y; ++i)
    {
        plaindata[3 * i] = imgbuf_[i].x;
        plaindata[3 * i + 1] = imgbuf_[i].y;
        plaindata[3 * i + 2] = imgbuf_[i].z;
    }

    io_.Write(filename_, plaindata, ImageIo::ImageDesc(res_.x, res_.y, 3));
}

void FileImagePlane::AddSample(float2 const& sample, float w, float3 value)
{
    float2 fimgpos = sample * res_; 
    int2   imgpos  = int2((int)std::floorf(fimgpos.x), (int)std::floorf(fimgpos.y));

    // Make sure we are in the image space as (<0.5f,<0.5f) might map outside of the image
    imgpos.x = (int)clamp(imgpos.x, 0, res_.x-1);
    imgpos.y = (int)clamp(imgpos.y, 0, res_.y-1);

    imgbuf_[res_.x * (res_.y - 1 - imgpos.y) + imgpos.x] += w * value;
}