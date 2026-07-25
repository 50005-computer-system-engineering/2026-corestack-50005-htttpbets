#ifndef TETRISBRAIN_MOVEMENT_H
#define TETRISBRAIN_MOVEMENT_H

#include <stdbool.h>
#include "state.h"

/* INPUTS */
bool moveLeft(GameState *state);

bool moveRight(GameState *state);

bool softDrop(GameState *state);

int hardDrop(GameState *state);

#endif