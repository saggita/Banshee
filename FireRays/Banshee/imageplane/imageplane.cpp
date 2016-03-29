#include "imageplane.h"
#include "../filter/imagefilter.h"

ImagePlane::~ImagePlane() = default;

void ImagePlane::AddSample(float2 const& pos, float3 const& value)
{
	// Get the filter
	auto* filter = GetImageFilter();

	if (filter)
	{
		// Obtain filter radius
		auto radius = filter->GetRadius();
		// Calculate closest pixel
		int2 intpos = int2((int)pos.x, (int)pos.y);
		// Iterate over the filter region
		for (int x = std::max(0, intpos.x - radius); 
		x <= std::max(intpos.x + radius, resolution().x - 1); 
			++x)
			for (int y = std::max(0, intpos.y - radius);
		y <= std::max(intpos.y + radius, resolution().y - 1);
			++y)
		{
			
			float2 center = float2(x + 0.5f, y + 0.5f);
			float w = filter->Evaluate(center - pos);

			WriteSample(int2(x, y), w * value);
		}
	}
	else
	{
		// Get integer coordinates by rounding and add the buffer
		int2 intpos = int2((int)pos.x, (int)pos.y);
		WriteSample(intpos, value);
	}
}
