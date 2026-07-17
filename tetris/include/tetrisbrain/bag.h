#ifndef TETRISBRAIN_BAG_H
#define TETRISBRAIN_BAG_H

#include "state.h"

// Helper method to shuffle arrays
void shuffleArray(int *array, int size);

// Shifts the upcoming bag forward and generates a new one
void refillBag(GameState *state);

// Spawns a new piece
void spawnNewPiece(GameState *state);

#endif