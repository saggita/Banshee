#ifndef CUSTOM_WORLDBUILDER_H
#define CUSTOM_WORLDBUILDER_H

#include <functional>
#include <memory>

#include "worldbuilder.h"

///< Custom world builder is a debug class 
///< allowing for programmer specified build functions
///< for the world.
///<
class CustomWorldBuilder: public WorldBuilder
{
    CustomWorldBuilder(std::function< std::unique_ptr<World>() > worldfunc)
        : worldfunc_(worldfunc)
    {
    }

    std::unique_ptr<World> Build() const
    {
        return worldfunc_();
    }

private:
    std::function< std::unique_ptr<World>() > worldfunc_;
};


#endif // CUSTOM_WORLDBUILDER_H