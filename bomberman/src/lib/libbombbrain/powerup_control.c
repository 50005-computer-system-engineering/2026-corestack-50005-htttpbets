#include "powerup_control.h"
#include <stdlib.h>
#include "map.h"

#define POWERUP_CHANCE 30



// Assume that we only call this when the tile is valid!
// TODO: Delay until explosion is over
void try_spawn_powerup(int x, int y) {
    // (1) Check spawn rate
    int rng = rand() % 100;
    if (rng < POWERUP_CHANCE)
        return;

    // (2) Get type of powerup
    int powerup_type = rand() % 2;
    if (powerup_type == 0)
        map[x][y] = POWERUP_FIRE;
    else
        map[x][y] = POWERUP_BOMB;
}