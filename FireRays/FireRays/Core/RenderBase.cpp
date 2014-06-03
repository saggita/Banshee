//
//  RenderBase.cpp
//  FireRays
//
//  Created by dmitryk on 02.02.14.
//
//

#include "RenderBase.h"

#include "TextureBase.h"


void RenderBase::AttachTexture(std::string const& name, std::shared_ptr<TextureBase> texture)
{
    textures_.insert(std::make_pair(name, texture));
    texturesDirty_ = true;
}

void RenderBase::DetachTexture(std::string const& name)
{
    TextureMap::const_iterator citer = textures_.find(name);
    if (citer != textures_.cend())
        textures_.erase(citer);
    texturesDirty_ = true;
}


void                                RenderBase::SetKernelProvider(std::shared_ptr<KernelProviderBase> kernelProvider)
{
    kernelProvider_ = kernelProvider;
}

std::shared_ptr<KernelProviderBase> RenderBase::GetKernelProvider() const
{
    return kernelProvider_;
}