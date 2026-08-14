#ifndef TILE_H
#define TILE_H
#include "raylib.h"

void tiles_init();
void tiles_draw();
void tiles_cleanup();

void draw_static_sprite(Texture2D texture, Vector2 position, float size, float rotation, Color tint);
#endif
