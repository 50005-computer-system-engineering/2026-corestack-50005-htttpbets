#ifndef INVENTORY_H
#define INVENTORY_H
#include "raylib.h"

typedef struct {
    int remaining_bombs;
    int num_bombs;
    int num_fires;

    float bomb_replenish_timer;
} InventoryStock;

void consume_bomb(InventoryStock* stock);
void inventory_update(InventoryStock* stock, float delta_time);
#endif
