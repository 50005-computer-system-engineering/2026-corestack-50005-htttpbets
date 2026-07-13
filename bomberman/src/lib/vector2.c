#include "vector2.h"
#include <math.h>

Vector2 Create(float x, float y) {
    Vector2 v;
    v.x = x;
    v.y = y;
    return v;
}

void Copy(Vector2 *dest, Vector2 src) {
    dest->x = src.x;
    dest->y = src.y;
}

void Add(Vector2 *dest, Vector2 v) {
    dest->x += v.x;
    dest->y += v.y;
}

void Subtract(Vector2 *dest, Vector2 v) {
    dest->x -= v.x;
    dest->y -= v.y;
}

void Multiply(Vector2 *dest, Vector2 v) {
    dest->x *= v.x;
    dest->y *= v.y;
}

void Divide(Vector2 *dest, Vector2 v) {
    dest->x /= v.x;
    dest->y /= v.y;
}

float Length(Vector2 v) {
    return sqrt(v.x * v.x + v.y * v.y);
}

float Dot(Vector2 v1, Vector2 v2) {
    return v1.x * v2.x + v1.y * v2.y;
}

float Distance(Vector2 v1, Vector2 v2) {
    return sqrt((v1.x - v2.x) * (v1.x - v2.x) 
        + (v1.y - v2.y) * (v1.y - v2.y));
}

Vector2 Normalize(Vector2 v) {
    float length = Length(v);
    return Create(v.x / length, v.y / length);
}

Vector2 Negate(Vector2 v) {
    return Create(-v.x, -v.y);
}