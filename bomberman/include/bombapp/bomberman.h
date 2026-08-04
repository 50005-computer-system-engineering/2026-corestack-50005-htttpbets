#ifndef BOMBERMAN_H
#define BOMBERMAN_H
#include <raylib.h>
#include "lib/libbombbrain/collision.h"
#include "spritesheet.h"
#include "lib/libbombbrain/inventory.h"

typedef struct {
    int direction;
    bool is_moving;

    // Follows direction index
    Spritesheet stand_sprites[4];
    Spritesheet walk_sprites[4];
    Spritesheet* curr_sprite;

    BoundBox box;

    // Inventory
    InventoryStock inventory;
} Bomberman;

Bomberman bomberman_create_default(Vector2 position);
void bomberman_update(Bomberman* bm);
void bomberman_draw(Bomberman* bm, Color tint);
void bomberman_cleanup(Bomberman* bm);
#endif