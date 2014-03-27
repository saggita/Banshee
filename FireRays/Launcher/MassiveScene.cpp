#include "MassiveScene.h"

#include "Mesh.h"
#include "eig3.h"
#include "utils.h"


std::shared_ptr<MassiveScene> MassiveScene::Create()
{
    return std::shared_ptr<MassiveScene>(new MassiveScene());
}

static SceneBase::Vertex MeshVertexToSceneVertex(Mesh::Vertex const& v)
{
    SceneBase::Vertex vtx;
    vtx.position = v.position;
    vtx.texcoord = v.texcoord;
    vtx.normal   = v.normal;
    return vtx;
}

	
MassiveScene::MassiveScene()
{
    BuildMaterials();
    
    {
        std::shared_ptr<Mesh> mesh_ptr = Mesh::CreateFromFile("../../../Resources/crytek-sponza/sponza.objm");
        //mesh_ptr->Rescale(0.1);
        Mesh::Vertex const* vertexData = mesh_ptr->GetVertexArrayPtr();
        unsigned const* indexData = mesh_ptr->GetIndexArrayPtr();

        for (int v = 0; v < mesh_ptr->GetVertexCount(); ++v)
        {
            vertices_.push_back(MeshVertexToSceneVertex(vertexData[v]));
        }

        for (int idx = 0; idx < mesh_ptr->GetIndexCount(); ++idx)
        {
            indices_.push_back(indexData[idx]);
        }

        unsigned material = 0;
        for (int idx = 0; idx < mesh_ptr->GetIndexCount()/3; ++idx)
            materials_.push_back(material);
    }

    unsigned startIdx = vertices_.size();

    Vertex v;

    v.position = vector3(28, 0, -1);
    v.normal   = vector3(0, -1, 0);
    v.texcoord = vector2(0, 0);

    vertices_.push_back(v);

    v.position = vector3(30, 0, -1);
    v.normal   = vector3(0, -1, 0);
    v.texcoord = vector2(1, 0);

    vertices_.push_back(v);

    v.position = vector3(30, 0,  1);
    v.normal   = vector3(0, -1, 0);
    v.texcoord = vector2(1, 1);

    vertices_.push_back(v);

    v.position = vector3(28, 0,  1);
    v.normal   = vector3(0, -1, 0);
    v.texcoord = vector2(0, 1);

    vertices_.push_back(v);

    indices_.push_back(startIdx);
    indices_.push_back(startIdx + 1);
    indices_.push_back(startIdx + 3);
    indices_.push_back(startIdx + 3);
    indices_.push_back(startIdx + 1);
    indices_.push_back(startIdx + 2);

    materials_.push_back(2);
    materials_.push_back(2);
}

MassiveScene::~MassiveScene()
{

}

MassiveScene::Vertex const*       MassiveScene::GetVertices() const
{
    return &vertices_[0];
}
unsigned int        MassiveScene::GetVertexCount() const
{
    return (unsigned int)vertices_.size();
}

unsigned int const* MassiveScene::GetIndices() const
{
    return &indices_[0];
}

unsigned int        MassiveScene::GetIndexCount() const
{
    return (unsigned int)indices_.size();
}

unsigned int const* MassiveScene::GetMaterials() const
{
    return &materials_[0];
}

void MassiveScene::BuildMaterials()
{
    MaterialRep materialRep;

    materialRep.eBsdf = 1;
    materialRep.vKe.x() = materialRep.vKe.y() = materialRep.vKe.z() = materialRep.vKe.w() = 0;
    materialRep.vKd.x() = materialRep.vKd.y() = materialRep.vKd.z() = materialRep.vKd.w() = 0.6;
    materialRep.vKs.x() = materialRep.vKs.y() = materialRep.vKs.z() = materialRep.vKs.w() = 0.0;
    materialReps_.push_back(materialRep);

    materialRep.eBsdf = 1;
    materialRep.vKe.x() = materialRep.vKe.y() = materialRep.vKe.z() = materialRep.vKe.w() = 0;
    materialRep.vKd.x() = materialRep.vKd.y() = materialRep.vKd.z() = materialRep.vKd.w() = 0.6;
    materialRep.vKs.x() = materialRep.vKs.y() = materialRep.vKs.z() = materialRep.vKs.w() = 0.0;
    materialReps_.push_back(materialRep);

    materialRep.eBsdf = 3;
    materialRep.vKe.x() = materialRep.vKe.y() = materialRep.vKe.z() = materialRep.vKe.w() = 1.0;
    materialRep.vKd.x() = materialRep.vKd.y() = materialRep.vKd.z() = materialRep.vKd.w() = 0.0;
    materialRep.vKs.x() = materialRep.vKs.y() = materialRep.vKs.z() = materialRep.vKs.w() = 0.0;
    materialReps_.push_back(materialRep);
}

unsigned int MassiveScene::GetMaterialRepCount() const
{
    return (unsigned int)materialReps_.size();
}

MassiveScene::MaterialRep const*  MassiveScene::GetMaterialReps() const
{
    return &materialReps_[0];
}


