#ifndef MAP_H
#define MAP_H

#include <raylib.h>

typedef enum {
    EMPTY = 0,
    PLAYER,
    BREAKABLE,
    WALL,
    BOMB,
    POWERUP_FIRE,
    POWERUP_BOMB,

    TILETYPE_COUNT
} TileType;

extern int map_size; // Width and height of map
extern TileType** map; // 2D Array of tiles

void map_generate(int num_players);
void map_free();
void map_debugprint();
#endif