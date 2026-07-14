#include "vector2int.h"
#include <math.h>

Vector2Int Create(int x, int y) {
    Vector2Int v;
    v.x = x;
    v.y = y;
    return v;
}

void Copy(Vector2Int *dest, Vector2Int src) {
    dest->x = src.x;
    dest->y = src.y;
}

void Add(Vector2Int *dest, Vector2Int v) {
    dest->x += v.x;
    dest->y += v.y;
}

void Subtract(Vector2Int *dest, Vector2Int v) {
    dest->x -= v.x;
    dest->y -= v.y;
}

void Multiply(Vector2Int *dest, Vector2Int v) {
    dest->x *= v.x;
    dest->y *= v.y;
}

void Divide(Vector2Int *dest, Vector2Int v) {
    dest->x /= v.x;
    dest->y /= v.y;
}

int Length(Vector2Int v) {
    return sqrt(v.x * v.x + v.y * v.y);
}

int Dot(Vector2Int v1, Vector2Int v2) {
    return v1.x * v2.x + v1.y * v2.y;
}

int Distance(Vector2Int v1, Vector2Int v2) {
    return sqrt((v1.x - v2.x) * (v1.x - v2.x) 
        + (v1.y - v2.y) * (v1.y - v2.y));
}

Vector2Int Normalize(Vector2Int v) {
    int length = Length(v);
    return Create(v.x / length, v.y / length);
}

Vector2Int Negate(Vector2Int v) {
    return Create(-v.x, -v.y);
}