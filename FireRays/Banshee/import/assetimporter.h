#ifndef ASSETIMPORTER_H
#define ASSETIMPORTER_H

#include <functional>

class Primitive;
class Light;
class Camera;
class Material;
class TextureSystem;

///< AssetImporter defines a callback interface for various 
///< asset file loaders. Set appropriate callbacks and call
///< Import() method to start import process
///<
class AssetImporter
{
public:
    AssetImporter(TextureSystem const& texsys)
        : onprimitive_(nullptr)
        , onlight_(nullptr)
        , oncamera_(nullptr)
        , onmaterial_(nullptr)
        , texsys_(texsys)
    {
    }

    // Destructor
    virtual ~AssetImporter(){}
    // Import asset
    virtual void Import() = 0;

    // New primitive callback
    std::function<void (Primitive*)> onprimitive_;
    // New light callback
    std::function<void (Light*)> onlight_;
    // New camera callback
    std::function<void (Camera*)> oncamera_;
    // New material callback
    std::function<int (Material*)> onmaterial_;

private:
    AssetImporter(AssetImporter const&);
    AssetImporter& operator = (AssetImporter const&);

protected:
    TextureSystem const& texsys_;
};


#endif //ASSETIMPORTER_H