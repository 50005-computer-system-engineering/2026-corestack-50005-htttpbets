#ifndef VECTOR2_H
#define VECTOR2_H
typedef struct {
    int x;
    int y;
} Vector2;

// Methods to manipulate Vector2
Vector2 vector2_add(Vector2 a, Vector2 b);
Vector2 vector2_subtract(Vector2 a, Vector2 b);
Vector2 vector2_multiply(Vector2 a, Vector2 b);
Vector2 vector2_divide(Vector2 a, Vector2 b);
Vector2 vector2_scale(Vector2 a, float scale);
float vector2_length(Vector2 a);
float vector2_distance(Vector2 a, Vector2 b);

// Fast Constructors
extern const Vector2 VECTOR2_ZERO;
extern const Vector2 VECTOR2_ONE;
#endif
