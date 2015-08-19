#include "assimp_assetimporter.h"

#include <map>
#include <algorithm>
#include <cassert>

#include "Importer.hpp"
#include "mesh.h"
#include "postprocess.h"
#include "scene.h"

#include "../material/emissive.h"
#include "../material/simplematerial.h"
#include "../bsdf/lambert.h"
#include "../primitive/mesh.h"
#include "../light/arealight.h"

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
    std::map<int, Material*> idx2mat;

    // Start with materials since we need to know
    // their indices when importing geometry
    for (int i = 0; i < (int)scene->mNumMaterials; ++i)
    {
        aiMaterial* material = scene->mMaterials[i];

        std::string kdmap ="";
        std::string nmap ="";
        aiString path;
        if(material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS) 
        {
            kdmap = path.data;
        }

        if(material->GetTexture(aiTextureType_HEIGHT, 0, &path) == AI_SUCCESS) 
        {
            nmap = path.data;
        }

        aiColor3D diffuse(0,0,0);
        material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);

        aiColor3D specular(0,0,0);
        material->Get(AI_MATKEY_COLOR_SPECULAR, specular);

        aiColor3D emission(0,0,0);
        material->Get(AI_MATKEY_COLOR_EMISSIVE, emission);

        // TODO: this is hack for Assimp not supporting Ke
        // material property in OBJ files
        // For now constant light power for materials called "light"
        aiString matname;
        material->Get(AI_MATKEY_NAME, matname);

        float3 kd = float3(diffuse.r, diffuse.g, diffuse.b);
        //float3 ks = float3(specular.r, specular.g, specular.b);
        float3 ke = float3(emission.r, emission.b, emission.g);

        Material* m = nullptr;

        if (ke.sqnorm() > 0.f || matname == aiString("light"))
        {
            m = new Emissive(0.6f * float3(60.f,55.f,40.f));
        }
        else
        {
            m = new SimpleMaterial(new Lambert(texsys_, kd, kdmap, nmap));
        }

        if (onmaterial_)
        {
            int idx = onmaterial_(m);
            ai2idx[material] = idx;
            idx2mat[idx] = m;
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

        if (onlight_ && idx2mat[mat]->IsEmissive())
        {
            for (size_t i = 0; i < mymesh->GetNumShapes(); ++i)
            {
                AreaLight* light = new AreaLight(i, *mymesh, *idx2mat[mat]);
                onlight_(light);
            }
        }
    }
}
