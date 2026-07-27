#ifndef BOMB_CONTROL_H
#define BOMB_CONTROL_H
#include <raylib.h>
#include "utils/queue.h"

typedef struct {
    Vector2 position;
    float timer;
} Bomb;
DEFINE_QUEUE(Bomb, Bombs, 500);
extern BombsQueue bombs_queue;

void init_bombs();
void tick_bombs(float delta_time);

// Validate and place bomb, returns true if successfully placed
bool place_bomb(Vector2 pos);
#endif