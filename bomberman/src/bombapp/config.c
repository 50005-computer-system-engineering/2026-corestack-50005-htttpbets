#include <raylib.h>
#include "config.h"

const int player_fps = 10;
const float player_scale = 2.0f;

const Config CONFIG = {
    .KEYS = {
        .MOVE_UP = KEY_W,
        .MOVE_DOWN = KEY_S,
        .MOVE_LEFT = KEY_A,
        .MOVE_RIGHT = KEY_D,
        .BOMB = KEY_SPACE,
        .EXIT = KEY_ESCAPE
    },

    .PHYSICS = {
        .PLAYER_SPEED = 300.0f
    },

    .ASSETS = {
        .PLAYER_STAND = {
            (SpritesheetAsset){
                .path = "../../assets/sprites/Bomber_Stand_Up.png",
                .cols = 3, .rows = 1, .scale = player_scale, .fps = player_fps, .should_loop = true
            },
            (SpritesheetAsset){
                .path = "../../assets/sprites/Bomber_Stand_Down.png",
                .cols = 3, .rows = 1, .scale = player_scale, .fps = player_fps, .should_loop = true
            },
            (SpritesheetAsset){
                .path = "../../assets/sprites/Bomber_Stand_Left.png",
                .cols = 3, .rows = 1, .scale = player_scale, .fps = player_fps, .should_loop = true
            },
            (SpritesheetAsset){
                .path = "../../assets/sprites/Bomber_Stand_Right.png",
                .cols = 3, .rows = 1, .scale = player_scale, .fps = player_fps, .should_loop = true
            }
        },

        .PLAYER_WALK = {
            (SpritesheetAsset){
                .path = "../../assets/sprites/Bomber_Walk_Up.png",
                .cols = 6, .rows = 1, .scale = player_scale, .fps = player_fps, .should_loop = true
            },
            (SpritesheetAsset){
                .path = "../../assets/sprites/Bomber_Walk_Down.png",
                .cols = 6, .rows = 1, .scale = player_scale, .fps = player_fps, .should_loop = true
            },
            (SpritesheetAsset){
                .path = "../../assets/sprites/Bomber_Walk_Left.png",
                .cols = 6, .rows = 1, .scale = player_scale, .fps = player_fps, .should_loop = true
            },
            (SpritesheetAsset){
                .path = "../../assets/sprites/Bomber_Walk_Right.png",
                .cols = 6, .rows = 1, .scale = player_scale, .fps = player_fps, .should_loop = true
            }
        },

        .PLAYER_WIN = {
            .path = "../../assets/sprites/Bomber_Win.png",
            .cols = 12, .rows = 1, .scale = player_scale, .fps = player_fps, .should_loop = true
        },
    }
};
