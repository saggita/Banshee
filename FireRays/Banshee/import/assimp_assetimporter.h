#ifndef ASSIMP_ASSETIMPORTER_H
#define ASSIMP_ASSETIMPORTER_H

#include <string>

#include "assetimporter.h"

///< AssetImporter defines a callback interface for various 
///< asset file loaders. Set appropriate callbacks and call
///< Import() method to start import process
///<
class AssimpAssetImporter : public AssetImporter
{
public:
    AssimpAssetImporter(TextureSystem const& texsys, std::string const& filename)
        : AssetImporter(texsys)
        , filename_(filename)
    {
    }

    void Import();

private:
    // Asset file name
    std::string filename_;
};


#endif //ASSIMP_ASSETIMPORTER_H