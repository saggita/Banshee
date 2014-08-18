#include "fileimageplane.h"

#include "../math/mathutils.h"
#include <cmath>
#include <cassert>


void FileImagePlane::Prepare()
{
}

void FileImagePlane::Finalize()
{
    std::vector<float> plaindata(res_.x * res_.y * 3);

    for (int i = 0; i < res_.x * res_.y; ++i)
    {
        plaindata[3 * i] = powf(imgbuf_[i].x, 1.f / 2.2f);
        plaindata[3 * i + 1] = powf(imgbuf_[i].y, 1.f / 2.2f);
        plaindata[3 * i + 2] = powf(imgbuf_[i].z, 1.f / 2.2f);
    }

    io_.Write(filename_, plaindata, ImageIo::ImageDesc(res_.x, res_.y, 3));
}

void FileImagePlane::AddSample(float2 const& sample, float w, float3 value)
{
    assert(!has_nans(value));

    float2 fimgpos = sample * res_; 
    int2   imgpos  = int2((int)std::floorf(fimgpos.x), (int)std::floorf(fimgpos.y));

    // Make sure we are in the image space as (<0.5f,<0.5f) might map outside of the image
    imgpos.x = (int)clamp((float)imgpos.x, 0.f, (float)res_.x-1);
    imgpos.y = (int)clamp((float)imgpos.y, 0.f, (float)res_.y-1);

    imgbuf_[res_.x * (res_.y - 1 - imgpos.y) + imgpos.x] += w * value;
}