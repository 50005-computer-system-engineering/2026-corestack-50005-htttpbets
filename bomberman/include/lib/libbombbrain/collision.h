#ifndef COLLISION_H
#define COLLISION_H
#include <raylib.h>
#include "map.h"

// Actual positions at top left, then visible position is moved in-between tiles
typedef struct {
    Vector2 position; // Top Left of box
    Vector2 size; // Width and Height of box (as a ratio of a tile, 0f - 1f)
} BoundBox;

void move_box(BoundBox *box, Vector2 moveAmt);
TileType get_tile_at_box(BoundBox *box);
#endif