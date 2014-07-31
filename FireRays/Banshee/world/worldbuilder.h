#ifndef WORLDBUILDER_H
#define WORLDBUILDER_H

#include <memory>

class World;

///< WorldBuilder class is an interface for different asset loaders.
///< They should override the single Build() method and produce Banshee World from there.
///< 
class WorldBuilder
{
public:

    virtual ~WorldBuilder(){}

    // Override this function to produce world from your assets
    virtual std::unique_ptr<World> Build() const = 0;

};


#endif // WORLDBUILDER