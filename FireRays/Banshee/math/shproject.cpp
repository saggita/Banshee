#include "shproject.h"
#include "sh.h"

#include "mathutils.h"

#include <vector>

///< The function projects latitude-longitude environment map to SH basis up to lmax band
void ShProjectEnvironmentMap(TextureSystem const& texsys, std::string const& texture, int lmax, float3* coeffs)
{
    // Resulting coefficients for RGB
    // std::vector<float3> coeffs(NumShTerms(lmax));
    // Temporary coefficients storage
    std::vector<float>  ylm(NumShTerms(lmax));

    // Get texture width and height
    TextureSystem::TextureDesc texdesc;
    texsys.GetTextureInfo(texture, texdesc);

    // Precompute sin and cos for the sphere
    std::vector<float> sintheta(texdesc.height);
    std::vector<float> costheta(texdesc.height);
    std::vector<float> sinphi(texdesc.width);
    std::vector<float> cosphi(texdesc.width);

    float thetastep = PI / texdesc.height;
    float phistep = 2.f*PI / texdesc.width;
    float theta0 = PI / texdesc.height / 2;
    float phi0 = 2.f*PI / texdesc.width / 2;

    for (int i = 0; i < texdesc.width; ++i)
    {
        sinphi[i] = std::sinf(phi0 + i * phistep);
        cosphi[i] = std::cosf(phi0 + i * phistep);
    }

    for (int i = 0; i < texdesc.height; ++i)
    {
        sintheta[i] = std::sinf(theta0 + i * thetastep);
        costheta[i] = std::cosf(theta0 + i * thetastep);
    }

    // Iterate over the pixels calculating Riemann sum
    for (int phi = 0; phi < texdesc.width; ++phi)
    {
        for (int theta = 0; theta < texdesc.height; ++theta)
        {
            // Construct direction vector
            float3 w = normalize(float3(sintheta[theta] * cosphi[phi], sintheta[theta] * sinphi[phi], costheta[theta]));

            // Construct uv sample coordinates
            float2 uv = float2((float)phi / texdesc.width, (float)theta / texdesc.height);

            // Sample environment map
            TextureSystem::Options opts;
            opts.wrapmode = TextureSystem::Options::kRepeat;
            opts.filter= TextureSystem::Options::kPoint;
            float3 le = texsys.Sample(texture, uv, float2());

            // Evaluate SH functions at w up to lmax band
            ShEvaluate(w, lmax, &ylm[0]);

            // Evaluate Riemann sum accouting for solid angle conversion (sin term)
            for (int i = 0; i < NumShTerms(lmax); ++i)
            {
                coeffs[i] += le * ylm[i] * sintheta[theta] * (PI / texdesc.height) * (2.f * PI / texdesc.width);
            }
        }
    }
}

///< The function projects latitude-longitude environment map to SH basis up to lmax band
void ShProjectEnvironmentMap(float3 const* envmap, int width, int height, int lmax, float3* coeffs)
{
    // Resulting coefficients for RGB
    // std::vector<float3> coeffs(NumShTerms(lmax));
    // Temporary coefficients storage
    std::vector<float>  ylm(NumShTerms(lmax));

    // Precompute sin and cos for the sphere
    std::vector<float> sintheta(height);
    std::vector<float> costheta(height);
    std::vector<float> sinphi(width);
    std::vector<float> cosphi(width);

    float thetastep = PI / height;
    float phistep = 2.f * PI / width;
    float theta0 = PI / height / 2;
    float phi0 = 2.f * PI / width / 2;

    for (int i = 0; i < width; ++i)
    {
        sinphi[i] = std::sinf(phi0 + i * phistep);
        cosphi[i] = std::cosf(phi0 + i * phistep);
    }

    for (int i = 0; i < height; ++i)
    {
        sintheta[i] = std::sinf(theta0 + i * thetastep);
        costheta[i] = std::cosf(theta0 + i * thetastep);
    }

    // Iterate over the pixels calculating Riemann sum
    for (int phi = 0; phi < width; ++phi)
    {
        for (int theta = 0; theta < height; ++theta)
        {
            // Construct direction vector
            float3 w = normalize(float3(sintheta[theta] * cosphi[phi], sintheta[theta] * sinphi[phi], costheta[theta]));

            // Construct uv sample coordinates
            float2 uv = float2((float)phi / width, (float)theta / height);

            // Sample environment map
            int iu = (int)floor(uv.x * width);
            int iv = (int)floor(uv.y * height);

            float3 le = envmap[width * iv + iu];

            // Evaluate SH functions at w up to lmax band
            ShEvaluate(w, lmax, &ylm[0]);

            // Evaluate Riemann sum accouting for solid angle conversion (sin term)
            for (int i = 0; i < NumShTerms(lmax); ++i)
            {
                coeffs[i] += le * ylm[i] * sintheta[theta] * (PI / height) * (2.f * PI / width);
            }
        }
    }
}

///< The function evaluates SH functions and dumps values to latitude-longitude map
void ShEvaluateAndDump(ImageIo& io, std::string const& filename,  int width, int height, int lmax, float3 const* coeffs)
{
    // Prepare image memory
    std::vector<float> imagedata(width * height * 3);

    // Allocate space for SH functions
    std::vector<float> ylm(NumShTerms(lmax));

    // Precalculate sin and cos terms 
    std::vector<float> sintheta(height);
    std::vector<float> costheta(height);
    std::vector<float> sinphi(width);
    std::vector<float> cosphi(width);

    float thetastep = PI / height;
    float phistep = 2.f*PI / width;
    float theta0 = PI / height / 2;
    float phi0= 2.f*PI / width / 2;

    for (int i = 0; i < width; ++i)
    {
        sinphi[i] = std::sinf(phi0 + i * phistep);
        cosphi[i] = std::cosf(phi0 + i * phistep);
    }

    for (int i = 0; i < height; ++i)
    {
        sintheta[i] = std::sinf(theta0 + i * thetastep);
        costheta[i] = std::cosf(theta0 + i * thetastep);
    }

    // Iterate thru image pixels
    for (int phi = 0; phi < width; ++phi)
    {
        for (int theta = 0; theta < height; ++theta)
        {
            // Calculate direction
            float3 w = normalize(float3(sintheta[theta] * cosphi[phi], sintheta[theta] * sinphi[phi], costheta[theta]));

            // Evaluate SH functions at w up to lmax band
            ShEvaluate(w, lmax, &ylm[0]);

            // Evaluate function injecting SH coeffs
            for (int i = 0; i < NumShTerms(lmax); ++i)
            {
                imagedata[theta * width * 3 + phi * 3] += ylm[i] * coeffs[i].x;
                imagedata[theta * width * 3 + phi * 3 + 1] += ylm[i] * coeffs[i].y;
                imagedata[theta * width * 3 + phi * 3 + 2] += ylm[i] * coeffs[i].z;
            }
        }
    }

    // Write image to file
    ImageIo::ImageDesc imgdesc(width, height, 3);
    io.Write(filename, imagedata, imgdesc);
}