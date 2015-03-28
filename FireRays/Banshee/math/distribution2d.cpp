#include "distribution2d.h"

#include "mathutils.h"

#include <algorithm>

Distribution2D::Distribution2D(int n, int m, float const* values)
: n_(n)
, m_(m)
, conddist_(m)
{
    // Calculate conditional sampling density given a row is choosen
    for (int i=0; i<m; ++i)
    {
        // For row i
        conddist_[i].reset(new Distribution1D(n, values + i*n));
    }
    
    // Calculate marginal sampling density for rows
    std::vector<float> funcint(m);
    for (int i=0; i<m; ++i)
    {
        funcint[i] = conddist_[i]->funcint_;
    }
    
    // Create marginal distribution
    marginaldist_.reset(new Distribution1D(m, &funcint[0]));
}


float2 Distribution2D::Sample2D(float2 u, float& pdf) const
{
    float2 sample;
    
    // Row and column pdf
    float rpdf, cpdf;
    
    // Sample marginal distribution for the row
    sample.y = marginaldist_->Sample1D(u.y, rpdf);

    // Convert to row index
    int rowidx = clamp((int)(sample.y * m_), 0, m_-1);
    
    // Sample conditional density for column
    sample.x = conddist_[rowidx]->Sample1D(u.x, cpdf);

    // Multiply pdfs
    pdf = rpdf * cpdf;

    return sample;
}

float Distribution2D::Pdf(float2 uv)
{
    int rowidx = clamp((int)(uv.y * m_), 0, m_-1);
    int colidx = clamp((int)(uv.x * n_), 0, n_-1);
    
    if (conddist_[rowidx]->funcint_ * marginaldist_->funcint_ == 0.f) return 0.f;
    
    return conddist_[rowidx]->funcvals_[colidx] * marginaldist_->funcvals_[rowidx] / (conddist_[rowidx]->funcint_ * marginaldist_->funcint_);
}