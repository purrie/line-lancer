#ifndef MATH_H_
#define MATH_H_

#include <raylib.h>

Vector2 Vector2Perp(Vector2 vec);
Vector2 Vector2PerpCounter(Vector2 vec);
Vector2 Vector2Slerp(Vector2 v1, Vector2 v2, Vector2 pivot, float t);
Rectangle RectangleUnion(Rectangle a, Rectangle b);


#endif // MATH_H_
