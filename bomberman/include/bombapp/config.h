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
    const SpritesheetAsset PLAYER_WALK[4];  // Up, Down, Left, Right
    const SpritesheetAsset PLAYER_WIN;

    const char* const TILE_EMPTY;
    const char* const TILE_BREAKABLE;
    const char* const TILE_WALL;
    const char* const TILE_BOMB;
    const char* const TILE_POWERUP_BOMB;
    const char* const TILE_POWERUP_FIRE;

    const char* const TILE_EXPLODE_CORE;
    const char* const TILE_EXPLODE_MID;
    const char* const TILE_EXPLODE_CAP;
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
    const Keybindings KEYS;
    const Assets ASSETS;
    const Physics PHYSICS;
    const DefaultSettings SETTINGS;
} Config;

// Single global instance
extern const Config CONFIG;
#endif
