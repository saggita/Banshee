#ifndef WORLD_H
#define WORLD_H

#include <memory>

// Forward declarations
class Primitive;

///< World class is a container for all entities for the scene. 
///< It hosts entities and is in charge of destroying them.
///<
class World
{
public:
    virtual ~World(){}





    std::unique_ptr<Primitive> accelerator_;
};


#endif // WORLD_H