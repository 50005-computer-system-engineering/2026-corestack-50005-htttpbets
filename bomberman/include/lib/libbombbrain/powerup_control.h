#ifndef POWERUP_CONTROL_H
#define POWERUP_CONTROL_H
#include <raylib.h>
#include "bomb_control.h"

void powerup_init();
void powerup_free();
void spawn_pending_powerups(ExplosionInfo* explosion);
void randomise_powerup(int x, int y);
#endif