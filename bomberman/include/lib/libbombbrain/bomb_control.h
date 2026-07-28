#ifndef BOMB_CONTROL_H
#define BOMB_CONTROL_H
#include <raylib.h>
#include "utils/queue.h"


#define BOMB_TIMER 1.0f
#define EXPLODE_LIFETIME 0.5f

typedef struct {
    Vector2 position;
    int spread; // Spread in each direction (1 by default, a '+' shape)
    float timer;
} BombInfo;

typedef struct {
    Vector2 center;
    int spread_amt[4]; // U/D/L/R Spread in each direction, each dir can be blocked
    Vector2* spread_positions; // Positions of spread tiles
    int spread_positions_size;
    float timer; // Lifetime
} ExplosionInfo;

DEFINE_QUEUE(BombInfo, Bombs, 500);
DEFINE_QUEUE(ExplosionInfo, Explodes, 500);

extern BombsQueue bombs_queue;
extern ExplodesQueue explodes_queue;

void init_bombs();
void tick_bombs(float delta_time);
void tick_explosions(float delta_time);

// Validate and place bomb, returns true if successfully placed
bool place_bomb(Vector2 pos, int spread);
#endif