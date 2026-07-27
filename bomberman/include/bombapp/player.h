#ifndef PLAYER_H
#define PLAYER_H
#include <raylib.h>

void player_init(void);
void player_update(void);
void player_draw(void);
void player_cleanup(void);
Vector2 player_getpos(void);
#endif