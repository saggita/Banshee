//
//  TextureBase.h
//  FireRays
//
//  Created by dmitryk on 27.01.14.
//
//

#ifndef TEXTUREBASE_H
#define TEXTUREBASE_H

class TextureBase
{
public:
    TextureBase(){}
    virtual ~TextureBase() = 0;
    virtual unsigned int GetWidth() const = 0;
    virtual unsigned int GetHeight() const = 0;
    virtual float const* GetData() const = 0;
    
private:
    TextureBase(TextureBase const&);
    TextureBase& operator= (TextureBase const&);
    
};

inline TextureBase::~TextureBase(){}

#endif
