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
    virtual unsigned int GetWidth() const = 0;
    virtual unsigned int GetHeight() const = 0;
    virtual float* GetData() const = 0;
    
private:
    TextureBase(TextureBase const&);
    TextureBase& operator= (TextureBase const&);
    
};



#endif
