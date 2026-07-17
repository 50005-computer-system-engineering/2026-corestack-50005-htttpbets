/* DRAW BOARD */

#ifndef TETRISU_RENDERER_H
#define TETRISU_RENDERER_H

#include "lib/libtetrisbrain/state.h"

// Renders the board and active piece to the terminal
// void drawBoard(GameState *state);
void drawBothBoards(GameState *p1, GameState *p2);

#endif