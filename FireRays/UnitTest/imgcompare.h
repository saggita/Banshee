#ifndef IMGCOMPARE_H
#define IMGCOMPARE_H

#include "imageio/oiioimageio.h"

class ImgCompare
{
public:
    // Pass I/O here
    ImgCompare(ImageIo& io)
    : io_ (io)
    {
    }
    
    // Image comparison statistics
    struct Statistics;
    
    // Comparator
    virtual void Compare(std::string const& img1, std::string const& img2, Statistics& stat, float eps = 0.f);
    
    // Image I/O
    ImageIo& io_;
};

struct ImgCompare::Statistics
{
    // true if sizes differ
    bool sizediff;
    // true if format differs
    bool formatdiff;
    // Number of different pixels if sizes are equal
    int ndiff;
};


#endif 
