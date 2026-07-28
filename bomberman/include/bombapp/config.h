#ifndef CONFIG_H
#define CONFIG_H

#include "spritesheet.h"

// Controls
typedef struct {
    const int MOVE_UP;
    const int MOVE_DOWN;
    const int MOVE_LEFT;
    const int MOVE_RIGHT;
    const int SPRINT;
    const int BOMB;
    const int EXIT;
} Keybindings;

// Asset paths
typedef struct {
    const SpritesheetAsset PLAYER_STAND[4]; // Up, Down, Left, Right
    const SpritesheetAsset PLAYER_WALK[4]; // Up, Down, Left, Right
    const SpritesheetAsset PLAYER_WIN;

    const char* TILE_EMPTY;
    const char* TILE_BREAKABLE;
    const char* TILE_WALL;
    const char* TILE_BOMB;
    const char* TILE_POWERUP_BOMB;
    const char* TILE_POWERUP_FIRE;

    const char* TILE_EXPLODE_CORE;
    const char* TILE_EXPLODE_MID;
    const char* TILE_EXPLODE_CAP;
} Assets;

typedef struct {
    const float PLAYER_SPEED;
    const float PLAYER_SPRINT_SPEED;
    const float TILE_SIZE;
    const float PICKUP_SIZE;
} Physics;

typedef struct {
    const float MUSIC_VOLUME;
    const float SFX_VOLUME;
} DefaultSettings;

typedef struct {
    Keybindings KEYS;
    Assets ASSETS;
    Physics PHYSICS;
    DefaultSettings SETTINGS;
} Config;

// Single global instance
extern const Config CONFIG;
#endif