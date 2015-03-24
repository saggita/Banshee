#include "imgcompare.h"

#include <cmath>

void ImgCompare::Compare(std::string const& img1, std::string const& img2, Statistics& stat, float eps)
{
    std::vector<float> imgdata1, imgdata2;
    ImageIo::ImageDesc imgdesc1, imgdesc2;
    
    io_.Read(img1, imgdata1, imgdesc1);
    io_.Read(img2, imgdata2, imgdesc2);
    
    if (imgdesc1.xres != imgdesc2.xres || imgdesc1.yres != imgdesc2.yres || imgdesc1.nchannels != imgdesc2.nchannels)
    {
        stat.sizediff = true;
        stat.ndiff = -1;
        return;
    }
    
    stat.sizediff = false;
    stat.ndiff = 0;
    
    for (int i=0; i<(int)imgdata1.size(); ++i)
    {
        if (std::abs(imgdata1[i] - imgdata2[i]) > eps)
        {
            ++stat.ndiff;
        }
    }
}
