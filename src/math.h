#ifndef MATH_H_
#define MATH_H_

#include <raylib.h>

Vector2 Vector2Perp(Vector2 vec);
Vector2 Vector2PerpCounter(Vector2 vec);
Vector2 Vector2Slerp(Vector2 v1, Vector2 v2, Vector2 pivot, float t);

float Vector2AngleHorizon(Vector2 direction);

Rectangle RectangleUnion(Rectangle a, Rectangle b);
Rectangle RectangleIntersection(Rectangle a, Rectangle b);
Rectangle RectangleEnvelop(Rectangle rect, Vector2 point);

#endif // MATH_H_
