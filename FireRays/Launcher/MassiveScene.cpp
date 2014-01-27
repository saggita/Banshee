#include "MassiveScene.h"

#include "Mesh.h"
#include "eig3.h"
#include "utils.h"

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
    {
        std::shared_ptr<Mesh> mesh_ptr = Mesh::CreateFromFile("../../../Resources/monkey.objm");
        //mesh_ptr->Rescale(0.5f);
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
    
    {
        std::shared_ptr<Mesh> mesh_ptr = Mesh::CreateFromFile("../../../Resources/sibenik.objm");
        Mesh::Vertex const* vertexData = mesh_ptr->GetVertexArrayPtr();
        unsigned const* indexData = mesh_ptr->GetIndexArrayPtr();

        unsigned baseIdx = vertices_.size();
        unsigned startIdx = indices_.size();
                
        for (int v = 0; v < mesh_ptr->GetVertexCount(); ++v)
        {
            vertices_.push_back(MeshVertexToSceneVertex(vertexData[v]));
        }
                
        for (int idx = 0; idx < mesh_ptr->GetIndexCount(); ++idx)
        {
            indices_.push_back(indexData[idx] + baseIdx);
        }
        
        unsigned material = 1;
        for (int idx = 0; idx < mesh_ptr->GetIndexCount()/3; ++idx)
            materials_.push_back(material);
    }
    
}

MassiveScene::~MassiveScene()
{

}

std::vector<SceneBase::Vertex> const& MassiveScene::GetVertices() const
{
	return	vertices_;
}

std::vector<unsigned int> const& MassiveScene::GetIndices() const
{
	return indices_;
}

std::shared_ptr<MassiveScene> MassiveScene::Create()
{
	return std::make_shared<MassiveScene>();
}

std::vector<unsigned int> const& MassiveScene::GetMaterials() const
{
    return materials_;
}


