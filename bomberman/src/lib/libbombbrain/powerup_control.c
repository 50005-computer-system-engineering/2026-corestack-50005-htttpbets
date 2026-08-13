#include "powerup_control.h"
#include <stdlib.h>
#include "map.h"
#include "bomb_control.h"

#define POWERUP_CHANCE 50

TileType** powerups_to_spawn;

void powerup_init()
{
    powerups_to_spawn = (TileType**)malloc(map_size * sizeof(TileType*));
    // Allocate space for each individual row
    for (int i = 0; i < map_size; i++) {
        powerups_to_spawn[i] = (TileType*)malloc(map_size * sizeof(TileType));

        // Initialise all values to EMPTY
        for (int j = 0; j < map_size; j++)
            powerups_to_spawn[i][j] = EMPTY;
    }
}

void powerup_free()
{
    if (powerups_to_spawn == NULL)
        return;

    // Free each row
    for (int i = 0; i < map_size; i++)
        free(powerups_to_spawn[i]);

    // Free array of row pointers
    free(powerups_to_spawn);
    powerups_to_spawn = NULL;
}

void spawn_pending_powerups(ExplosionInfo* explosion)
{
    if (powerups_to_spawn == NULL)
        return;

    for (int i = 0; i < explosion->spread_positions_size; i++) {
        int x = (int)explosion->spread_positions[i].x;
        int y = (int)explosion->spread_positions[i].y;

        if (powerups_to_spawn[x][y] == EMPTY)
            continue;
        map[x][y] = powerups_to_spawn[x][y];
        powerups_to_spawn[x][y] = EMPTY;
    }
}

// Assume that we only call this when the tile is valid!
void randomise_powerup(int x, int y)
{
    // (1) Check spawn rate
    int rng = rand() % 100;
    if (rng > POWERUP_CHANCE)
        return;

    // (2) Get type of powerup
    // Add to buffer to be spawned later after explosions
    int powerup_type = rand() % 2;
    if (powerup_type == 0)
        powerups_to_spawn[x][y] = POWERUP_FIRE;
    else
        powerups_to_spawn[x][y] = POWERUP_BOMB;
}
