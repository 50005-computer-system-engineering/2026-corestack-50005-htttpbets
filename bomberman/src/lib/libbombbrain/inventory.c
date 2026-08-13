#include "inventory.h"
#define BOMB_REPLENISH_TIME 1.0f

void consume_bomb(InventoryStock* stock)
{
    stock->remaining_bombs--;
    stock->bomb_replenish_timer = BOMB_REPLENISH_TIME;
}

void inventory_update(InventoryStock* stock, float delta_time)
{
    if (stock->remaining_bombs >= stock->num_bombs)
        return;

    stock->bomb_replenish_timer -= delta_time;
    if (stock->bomb_replenish_timer > 0)
        return;
    stock->remaining_bombs += 1;
    stock->bomb_replenish_timer = BOMB_REPLENISH_TIME;
}
