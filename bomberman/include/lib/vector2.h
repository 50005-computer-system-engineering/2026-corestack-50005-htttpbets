#ifndef VECTOR2_H
#define VECTOR2_H

typedef struct {
    float x;
    float y;
} Vector2;

Vector2 Create(float x, float y);
void Copy(Vector2 *dest, Vector2 src);
void Add(Vector2 *dest, Vector2 v);
void Subtract(Vector2 *dest, Vector2 v);
void Multiply(Vector2 *dest, Vector2 v);
void Divide(Vector2 *dest, Vector2 v);
float Length(Vector2 v);
float Dot(Vector2 v1, Vector2 v2);
float Distance(Vector2 v1, Vector2 v2);
Vector2 Normalize(Vector2 v);
Vector2 Negate(Vector2 v);
#endif