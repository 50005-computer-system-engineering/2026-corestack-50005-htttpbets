#include "bomb_control.h"
#include "map.h"
#include <stdio.h>

#define BOMB_TIMER 1

void init_bombs() {
    Bombs_init(&bombs_queue);
}

void tick_bombs(float delta_time) {
    if (Bombs_empty(&bombs_queue))
        return;

    for (int i = bombs_queue.front + 1; i < bombs_queue.rear; i++) {
        Bomb *bomb = &bombs_queue.items[i];
        bomb->timer -= delta_time;

        // Explode bomb if timer is up!
        if (bomb->timer <= 0) {
            // Remove bomb from map
            printf("Removing bomb at %d %d\n", (int)bomb->position.x, (int)bomb->position.y);
            map[(int)bomb->position.x][(int)bomb->position.y] = EMPTY;
            Bombs_dequeue(&bombs_queue);

            // TODO: Trigger spread!
        }
    }
}

bool place_bomb(Vector2 pos) {
    // Check if bomb can be placed
    // (1) Check for empty tile
    TileType tile = map[(int)pos.x][(int)pos.y];
    printf("tile: %d\n", tile);
    if (tile != EMPTY && tile != PLAYER)
        return false;

    // (2) Check if we still have bombs
    // TODO

    // (3) Place bomb
    map[(int)pos.x][(int)pos.y] = BOMB;
    Bomb new_bomb = (Bomb){pos, BOMB_TIMER};
    Bombs_enqueue(&bombs_queue, new_bomb);

    return true;
}