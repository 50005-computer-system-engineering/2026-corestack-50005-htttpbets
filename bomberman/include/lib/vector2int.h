#ifndef VECTOR2_INT_H
#define VECTOR2_INT_H

// Provide the equivalent of RayLib's Vector2 type for ints
typedef struct {
    int x;
    int y;
} Vector2Int;

Vector2Int Create(int x, int y);
void Copy(Vector2Int *dest, Vector2Int src);
void Add(Vector2Int *dest, Vector2Int v);
void Subtract(Vector2Int *dest, Vector2Int v);
void Multiply(Vector2Int *dest, Vector2Int v);
void Divide(Vector2Int *dest, Vector2Int v);
int Length(Vector2Int v);
int Dot(Vector2Int v1, Vector2Int v2);
int Distance(Vector2Int v1, Vector2Int v2);
Vector2Int Normalize(Vector2Int v);
Vector2Int Negate(Vector2Int v);
#endif