#ifndef RENDERER_H
#define RENDERER_H

class World;

///< The main abstraction in the system encapsulating
///< rendering algorithm. The renderer is supposed to
///< produce an image given World instance.
///<
class Renderer
{
public:
    Renderer(){}
    virtual ~Renderer(){}

    virtual void Render(World const& world) const;

protected:

    Renderer(Renderer const&);
    Renderer& operator=(Renderer const&);
};

#endif //RENDERER_H
