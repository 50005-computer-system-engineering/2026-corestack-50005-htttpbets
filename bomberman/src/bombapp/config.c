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
        .SPRINT = KEY_LEFT_SHIFT,
        .BOMB = KEY_SPACE,
        .EXIT = KEY_ESCAPE
    },

    .PHYSICS = {
        .TILE_SIZE = 120.0f,
        .PICKUP_SIZE = 80.0f,
        .PLAYER_SPEED = 3.0f,
        .PLAYER_SPRINT_SPEED = 5.5f
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

        .TILE_EMPTY = "../../assets/tiles/Tile_Grass.png",
        .TILE_BREAKABLE = "../../assets/tiles/Tile_Breakable.png",
        .TILE_WALL = "../../assets/tiles/Tile_Unbreakable.png",
        .TILE_BOMB = "../../assets/tiles/Bomb.png",
        .TILE_POWERUP_BOMB = "../../assets/tiles/Powerup_Bomb.png",
        .TILE_POWERUP_FIRE = "../../assets/tiles/Powerup_Fire.png",

        .TILE_EXPLODE_CORE = "../../assets/tiles/Explode_Core.png",
        .TILE_EXPLODE_MID = "../../assets/tiles/Explode_Mid.png",
        .TILE_EXPLODE_CAP = "../../assets/tiles/Explode_Cap.png",
    },

    .SETTINGS = {
        .MUSIC_VOLUME = 0.7f,
        .SFX_VOLUME = 1.0f
    }
};
