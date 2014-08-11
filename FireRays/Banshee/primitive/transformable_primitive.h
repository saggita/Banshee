#ifndef TRANSFORMABLE_PRIMITIVE_H
#define TRANSFORMABLE_PRIMITIVE_H

#include "primitive.h"

#include "../math/matrix.h"

///< Partial implementation of Primitive inetrface which
///< allows setting local to world transform.
///<
class TransformablePrimitive : public Primitive
{
public:
    TransformablePrimitive(matrix const& worldmat, matrix const& worldmatinv)
    : worldmat_(worldmat)
    , worldmatinv_(worldmatinv)
    {
    }
    
protected:
    matrix worldmat_;
    matrix worldmatinv_;
};

#endif // TRANSFORMABLE_PRIMITIVE_H
