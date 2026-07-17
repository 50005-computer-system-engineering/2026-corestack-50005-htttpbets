#ifndef TETRISBRAIN_ENGINE_H
#define TETRISBRAIN_ENGINE_H

#include "state.h"

// Check for t-spin; return 1 if valid, 0 if not
int checkTSpin(GameState *state);

// Repeat function to advance the game
int tickGame(GameState *state);

#endif