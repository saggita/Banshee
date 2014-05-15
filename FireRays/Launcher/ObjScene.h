//
//  ObjScene.h
//  Launcher
//
//  Created by dmitryk on 11.05.14.
//
//

#ifndef __Launcher__ObjScene__
#define __Launcher__ObjScene__

#include <memory>
#include <vector>
#include <string>

#include "FireRays.h"

class aiScene;
class ObjScene : public SceneBase
{
public:
	static std::shared_ptr<ObjScene> Create(std::string const& fileName);
    ~ObjScene();
	
    Vertex const*       GetVertices() const;
    unsigned int        GetVertexCount() const;
    unsigned int const* GetIndices() const;
    unsigned int        GetIndexCount() const;
    unsigned int const* GetMaterials() const;
    
    unsigned int        GetMaterialRepCount() const;
    MaterialRep const*  GetMaterialReps() const;
	
private:
    
    void BuildMaterials(aiScene const* scene);
    
    ObjScene(std::string const& fileName);
    
	std::vector<Vertex> vertices_;
	std::vector<unsigned int> indices_;
    std::vector<unsigned int> materials_;
    std::vector<MaterialRep>  materialReps_;
};


#endif /* defined(__Launcher__ObjScene__) */
