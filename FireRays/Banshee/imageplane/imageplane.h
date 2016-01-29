/*
    Banshee and all code, documentation, and other materials contained
    therein are:

        Copyright 2013 Dmitry Kozlov
        All Rights Reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:
        * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
        * Neither the name of the software's owners nor the names of its
        contributors may be used to endorse or promote products derived from
        this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
    (This is the Modified BSD License)
*/
#ifndef IMAGEPLANE_H
#define IMAGEPLANE_H

#include "../math/float3.h"
#include "../math/int2.h"

#include <memory>

class ImageFilter;

///< ImagePlane class represents an image plane and
///< is designed for the Renderer to write its result.
///< Note that default image plane doesn't guarantee
///< atomicity of operations.
///<
class ImagePlane
{
public:
	// Image filter can be nullptr, in this case simple box filter is used
	ImagePlane(int2 const& res, ImageFilter* imgfilter);

	// Destructor
    virtual ~ImagePlane(){}

    // This method is called by the renderer prior to adding samples
    virtual void Prepare(){}

    // This method is called by the renderer after adding all the samples
    virtual void Finalize(){}

    // This is used by the renderer to decide on the number of samples needed
    int2 resolution() const;

	// Add weighted color contribution to the image plane
	// pos should be in the range of [0..res.x]x[0..res.y]
	// 
	void AddSample(float2 const& pos, float3 const& value);

protected:
	// Add sample to the pixel at position pos
	// pos should be in the range of [0..res.x]x[0..res.y]
	virtual void WriteSample(int2 const& pos, float3 const& value) = 0;
	
	// Access to image filter
	ImageFilter const* GetImageFilter() const;

private:
	// Image filter to use
	std::unique_ptr<ImageFilter> imagefilter_;
	// Resolution
	int2 res_;
};

inline ImagePlane::ImagePlane(int2 const& res, ImageFilter* imgfilter)
	: res_(res)
	, imagefilter_(imgfilter)
{
}

inline int2 ImagePlane::resolution() const
{
	return res_;
}

inline ImageFilter const* ImagePlane::GetImageFilter() const
{
	return imagefilter_.get();
}



#endif