#ifndef CONFIG_H
#define CONFIG_H

#include "spritesheet.h"

// Controls
typedef struct {
    const int MOVE_UP;
    const int MOVE_DOWN;
    const int MOVE_LEFT;
    const int MOVE_RIGHT;
    const int BOMB;
    const int EXIT;
} Keybindings;

typedef struct {
    const SpritesheetAsset PLAYER_STAND[4]; // Up, Down, Left, Right
    const SpritesheetAsset PLAYER_WALK[4]; // Up, Down, Left, Right
    const SpritesheetAsset PLAYER_WIN;
} Assets;

typedef struct {
    const float PLAYER_SPEED;
} Physics;

typedef struct {
    Keybindings KEYS;
    Assets ASSETS;
    Physics PHYSICS;
} Config;

// Single global instance
extern const Config CONFIG;
#endif