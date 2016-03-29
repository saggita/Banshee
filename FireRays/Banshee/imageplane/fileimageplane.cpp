#include "fileimageplane.h"

#include "../math/mathutils.h"
#include <cmath>
#include <cassert>

FileImagePlane::~FileImagePlane() = default;

void FileImagePlane::Prepare()
{
}

void FileImagePlane::Finalize()
{
    auto res = resolution();
    std::vector<float> plaindata(res.x * res.y * 3);

    for (int i = 0; i < res.x * res.y; ++i)
    {
        float w = m_imgbuf[i].w;
        plaindata[3 * i] = powf(m_imgbuf[i].x / w, 1.f / 2.2f);
        plaindata[3 * i + 1] = powf(m_imgbuf[i].y / w, 1.f / 2.2f);
        plaindata[3 * i + 2] = powf(m_imgbuf[i].z / w, 1.f / 2.2f);
    }

    m_io.Write(m_filename, plaindata, ImageIo::ImageDesc(res.x, res.y, 3));
}

void FileImagePlane::WriteSample(int2 const& pos, float3 const& value)
{
    auto res = resolution();
    m_imgbuf[res.x * (res.y - 1 - pos.y) + pos.x] += value;
    m_imgbuf[res.x * (res.y - 1 - pos.y) + pos.x].w += 1.f;
}
