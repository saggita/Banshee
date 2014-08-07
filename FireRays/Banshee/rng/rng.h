#ifndef RNG_H
#define RNG_H

///< An interface for random number generators
///<
class Rng
{
public:
    // Destructor
    virtual ~Rng() {}

    // Generate float
    virtual float NextFloat() const = 0;

    // Generate uint
    virtual unsigned NextUint() const = 0;
};


#endif // RNG_H