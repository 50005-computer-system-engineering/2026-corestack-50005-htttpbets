#include <raylib.h>
#include "bombapp/config.h"

const Config CONFIG = {
    .KEYS = {
        .MOVE_UP = KEY_W,
        .MOVE_DOWN = KEY_S,
        .MOVE_LEFT = KEY_A,
        .MOVE_RIGHT = KEY_D,
        .BOMB = KEY_SPACE,
        .EXIT = KEY_ESCAPE
    }
};
