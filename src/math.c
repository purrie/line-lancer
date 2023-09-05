#include "math.h"
#include <raymath.h>


Vector2 Vector2Perp(Vector2 vec) {
   return (Vector2){
        .x = vec.y,
        .y = vec.x * -1.0f
    };
}
Vector2 Vector2PerpCounter(Vector2 vec) {
    return (Vector2) {
        .x = vec.y * -1.0f,
        .y = vec.x
    };
}
Vector2 Vector2Slerp(Vector2 v1, Vector2 v2, Vector2 pivot, float t) {
    Vector2 p0 = Vector2Subtract(v1, pivot);
    Vector2 p1 = Vector2Subtract(v2, pivot);
    float angle = Vector2Angle(p0, p1);
    float sin_angle = sinf(angle);
    float sin_angle_t = sinf(angle * t);
    float sin_angle_nt = sinf((1.0f - t) * angle);
    float a = sin_angle_nt / sin_angle;
    float b = sin_angle_t / sin_angle;
    p0 = Vector2Scale(p0, a);
    p1 = Vector2Scale(p1, b);
    p0 = Vector2Add(p0, p1);
    p0 = Vector2Add(p0, pivot);
    return p0;
}

Rectangle RectangleUnion(Rectangle a, Rectangle b) {
    if (b.x < a.x) {
        float dif = a.x - b.x;
        a.x -= dif;
        a.width += dif;
    }
    if (b.y < a.y) {
        float dif = a.y - b.y;
        a.y -= dif;
        a.height += dif;
    }
    float x1 = a.x + a.width;
    float x2 = b.x + b.width;
    if (x2 > x1) {
        a.width += x2 - x1;
    }
    float y1 = a.y + a.height;
    float y2 = b.y + b.height;
    if (y2 > y1) {
        a.height += y2 - y1;
    }
    return a;
}
Rectangle RectangleIntersection(Rectangle a, Rectangle b) {
    Vector2 top_left, bottom_right;

    top_left.x = a.x > b.x ? a.x : b.x;
    top_left.y = a.y > b.y ? a.y : b.y;
    bottom_right.x = (a.x + a.width) < (b.x + b.width) ? a.x + a.width : b.x + b.width;
    bottom_right.y = (a.y + a.height) < (b.y + b.height) ? a.y + a.height : b.y + b.height;

    if (top_left.x > bottom_right.x || top_left.y > bottom_right.y)
        return (Rectangle){0};

    bottom_right = Vector2Subtract(bottom_right, top_left);

    return (Rectangle){ top_left.x, top_left.y, bottom_right.x, bottom_right.y };
}
Rectangle RectangleEnvelop(Rectangle rect, Vector2 point) {
    if (point.x < rect.x) {
        float diff = rect.x - point.x;
        rect.x -= diff;
        rect.width += diff;
    }
    else if (point.x > rect.x + rect.width) {
        float width = point.x - rect.x;
        rect.width = width;
    }

    if (point.y < rect.y) {
        float diff = rect.y - point.y;
        rect.y -= diff;
        rect.height += diff;
    }
    else if (point.y > rect.y + rect.height) {
        float height = point.y - rect.y;
        rect.height = height;
    }

    return rect;
}
