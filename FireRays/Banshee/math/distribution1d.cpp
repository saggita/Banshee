#include "distribution1d.h"
#include "mathutils.h"

#include <algorithm>
#include <cassert>

Distribution1D::Distribution1D(int numsegments, float const* values)
: numsegments_(numsegments)
, funcvals_(numsegments)
, cdf_(numsegments + 1)
{
    // Copy function values
    std::copy(values, values + numsegments, funcvals_.begin());
    
    // Calculate CDF
    cdf_[0] = 0.f;
    for (int i=1; i<numsegments+1; ++i)
    {
        cdf_[i] = cdf_[i-1] + funcvals_[i-1] / numsegments;
    }
    
    // Calculate normalizer
    funcint_ = cdf_[numsegments];

    // Normalize CDF
    for (int i=0; i<numsegments+1; ++i)
    {
        cdf_[i] /= funcint_;
    }
}


float Distribution1D::Sample1D(float u, float& pdf) const
{
    // Find the segment here u lies
    auto segiter = std::lower_bound(cdf_.cbegin() , cdf_.cend(), u);

    // Find segment index : clamp it as last may be returned for 1
    int segidx = clamp((int)std::distance(cdf_.cbegin(), segiter), 1, numsegments_+1);

    assert(u > cdf_[segidx-1] && u <= cdf_[segidx]);

    // Find lerp coefficient
    float du = (u - cdf_[segidx-1]) / (cdf_[segidx] - cdf_[segidx-1]);

    // Calc pdf
    pdf = funcvals_[segidx-1] / funcint_;

    // Return corresponding value
    return (segidx - 1 + du) / numsegments_;
}

float Distribution1D::Pdf(float u)
{
    // Find the segment here u lies
    auto segiter = std::lower_bound(cdf_.cbegin() , cdf_.cend(), u);
    
    // Find segment index : clamp it as last may be returned for 1
    int segidx = clamp((int)std::distance(cdf_.cbegin(), segiter), 1, numsegments_+1);
    
    // Calc pdf
    return funcvals_[segidx-1] / funcint_;
}