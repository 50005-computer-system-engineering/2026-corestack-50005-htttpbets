#ifndef COLLISION_H
#define COLLISION_H
#include "raylib.h"
#include <raymath.h>

// Actual positions at top left, then visible position is moved in-between tiles
typedef struct {
    Vector2 position; // Top Left of box
    Vector2 size;     // Width and Height of box (as a ratio of a tile, 0f - 1f)
} BoundBox;

Vector4 get_all_overlapping_tiles(float x, float y, Vector2 size);
void move_box(BoundBox* box, Vector2 move_amt);
Vector2 get_center_box(BoundBox* box);
#endif
