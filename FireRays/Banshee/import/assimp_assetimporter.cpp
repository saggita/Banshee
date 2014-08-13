#include "assimp_assetimporter.h"

#include <map>
#include <algorithm>
#include <cassert>

#include "Importer.hpp"
#include "Mesh.h"
#include "postprocess.h"
#include "scene.h"

#include "../material/matte.h"
#include "../primitive/mesh.h"

using namespace Assimp;

void AssimpAssetImporter::Import()
{
    Importer importer;

    const aiScene* scene = importer.ReadFile(
        filename_,
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType |
        aiProcess_GenNormals |
        aiProcess_GenUVCoords
        );

    if( !scene)
    {
        throw std::runtime_error("AssimpAssetImporter: Cannot load asset file");
    }

    // Map to track assimp to material ID conversion
    std::map<aiMaterial*, int> ai2idx;

    // Start with materials since we need to know
    // their indices when importing geometry
    for (int i = 0; i < (int)scene->mNumMaterials; ++i)
    {
        aiMaterial* material = scene->mMaterials[i];

        aiColor3D diffuse(0,0,0);
        material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);

        float3 kd = float3(diffuse.r, diffuse.g, diffuse.b);
        Material* m = new Matte(texsys_, kd);

        if (onmaterial_)
        {
            ai2idx[material] = onmaterial_(m);
        }
    }


    // Handle meshes next
    std::vector<int> indices;
    std::vector<int> materials;
    for (int m = 0; m < (int)scene->mNumMeshes; ++m)
    {
        const aiMesh* mesh = scene->mMeshes[m];

        // Collect mesh indices
        materials.resize(mesh->mNumFaces);
        indices.resize(mesh->mNumFaces * 3);
        for (int i = 0; i < (int)mesh->mNumFaces; ++i)
        {
            indices[3 * i] = mesh->mFaces[i].mIndices[0];
            indices[3 * i + 1] = mesh->mFaces[i].mIndices[1];
            indices[3 * i + 2] = mesh->mFaces[i].mIndices[2];
        }

        int mat = ai2idx[scene->mMaterials[mesh->mMaterialIndex]];

        assert (mat >= 0);

        std::fill(materials.begin(), materials.end(), mat);

        Mesh* mymesh = new Mesh(&mesh->mVertices[0].x, mesh->mNumVertices, sizeof(aiVector3D),
            &mesh->mNormals[0].x, mesh->mNumVertices, sizeof(aiVector3D),
            &mesh->mTextureCoords[0][0].x, mesh->mNumVertices, sizeof(aiVector3D),
            &indices[0], sizeof(int),
            &indices[0], sizeof(int),
            &indices[0], sizeof(int),
            &materials[0], sizeof(int),
            mesh->mNumFaces);

        if (onprimitive_)
        {
            onprimitive_(mymesh);
        }
    }
}