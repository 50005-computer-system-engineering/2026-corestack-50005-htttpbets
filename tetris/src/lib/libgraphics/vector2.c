#include "vector2.h"
#include <math.h>

const Vector2 VECTOR2_ZERO = {0, 0};
const Vector2 VECTOR2_ONE = {1, 1};

Vector2 vector2_add(Vector2 a, Vector2 b)
{
    Vector2 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

Vector2 vector2_subtract(Vector2 a, Vector2 b)
{
    Vector2 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}

Vector2 vector2_multiply(Vector2 a, Vector2 b)
{
    Vector2 result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    return result;
}

Vector2 vector2_divide(Vector2 a, Vector2 b)
{
    Vector2 result;
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    return result;
}

Vector2 vector2_scale(Vector2 a, float scale)
{
    Vector2 result;
    result.x = a.x * scale;
    result.y = a.y * scale;
    return result;
}

float vector2_length(Vector2 a)
{
    return sqrt(a.x * a.x + a.y * a.y);
}

float vector2_distance(Vector2 a, Vector2 b)
{
    return sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}