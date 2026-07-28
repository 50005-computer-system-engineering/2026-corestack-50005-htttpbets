#include "bomb_control.h"
#include "map.h"
#include <stdlib.h>
#include "lib/libeventbus.h"
#include "events.h"

#define BOMB_TIMER 1.0f
#define EXPLODE_LIFETIME 0.5f
#define POWERUP

BombsQueue bombs_queue;
ExplodesQueue explodes_queue;

void handle_spread(int x, int y, ExplosionInfo* explosion) {
    // TODO
    // TileType tile = map[x][y];
    // if (tile == BREAKABLE)
    //     try_spawn_powerup(x, y);

    map[x][y] = EXPLOSION;
    explosion->spread_positions[explosion->spread_positions_size] = (Vector2){x, y};
    explosion->spread_positions_size++;
}

void trigger_explosion(BombInfo bomb) {
    event_bus_trigger(EVENT_BOMB_EXPLODED, &(TileEventArgs){.x = (int)bomb.position.x, .y = (int)bomb.position.y});

    // Add to explosions
    ExplosionInfo explosion = { 0 };
    explosion.center = bomb.position;
    explosion.timer = EXPLODE_LIFETIME;

    // Calculate spread for each dir, ensure not blocked by corners or walls
    int x = (int)bomb.position.x;
    int y = (int)bomb.position.y;
    explosion.spread_positions = (Vector2*)malloc(bomb.spread * sizeof(Vector2));

    // Up
    for (int i = 1; i <= bomb.spread; i++) {
        if (y - i <= 0 || map[x][y - i] == WALL) 
            break;
        explosion.spread_amt[0] = i;
        handle_spread(x, y - i, &explosion);
    }

    // Down
    for (int i = 1; i <= bomb.spread; i++) {
        if (y + i >= map_size || map[x][y + i] == WALL)
            break;
        explosion.spread_amt[1] = i;
        handle_spread(x, y + i, &explosion);
    }

    // Left
    for (int i = 1; i <= bomb.spread; i++) {
        if (x - i <= 0 || map[x - i][y] == WALL)
            break;
        explosion.spread_amt[2] = i;
        handle_spread(x - i, y, &explosion);
    }

    // Right
    for (int i = 1; i <= bomb.spread; i++) {
        if (x + i >= map_size || map[x + i][y] == WALL)
            break;
        explosion.spread_amt[3] = i;
        handle_spread(x + i, y, &explosion);
    }
    Explodes_enqueue(&explodes_queue, explosion);
}

void init_bombs() {
    Bombs_init(&bombs_queue);
    Explodes_init(&explodes_queue);
}

void tick_bombs(float delta_time) {
    if (Bombs_empty(&bombs_queue))
        return;

    for (int i = bombs_queue.front + 1; i < bombs_queue.rear; i++) {
        BombInfo *bomb = &bombs_queue.items[i];
        bomb->timer -= delta_time;

        // Explode bomb if timer is up!
        if (bomb->timer <= 0) {
            // Remove bomb from map
            map[(int)bomb->position.x][(int)bomb->position.y] = EMPTY;
            Bombs_dequeue(&bombs_queue);

            // Trigger explosion!
            trigger_explosion(*bomb);
        }
    }
}

bool place_bomb(Vector2 pos, int spread) {
    // Check if bomb can be placed
    // (1) Check for empty tile
    TileType tile = map[(int)pos.x][(int)pos.y];
    if (tile != EMPTY && tile != PLAYER)
        return false;

    // (2) Check if we still have bombs
    // TODO

    // (3) Place bomb
    map[(int)pos.x][(int)pos.y] = BOMB;
    BombInfo new_bomb = (BombInfo){pos, spread, BOMB_TIMER};
    Bombs_enqueue(&bombs_queue, new_bomb);

    return true;
}

void tick_explosions(float delta_time) {
    if (Explodes_empty(&explodes_queue))
        return;

    for (int i = explodes_queue.front + 1; i < explodes_queue.rear; i++) {
        ExplosionInfo *explosion = &explodes_queue.items[i];
        explosion->timer -= delta_time;

        // Remove explosion after time is up
        if (explosion->timer <= 0) {
            map[(int)explosion->center.x][(int)explosion->center.y] = EMPTY;
            for (int i = 0; i < explosion->spread_positions_size; i++) {
                map[(int)explosion->spread_positions[i].x][(int)explosion->spread_positions[i].y] = EMPTY;
            }

            Explodes_dequeue(&explodes_queue);
        }
    }
}