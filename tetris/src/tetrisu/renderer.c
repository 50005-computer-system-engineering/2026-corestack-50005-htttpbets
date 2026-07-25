#include <stdio.h>
#include <stdbool.h>
#include "lib/libtetrisbrain/board_control.h"
#include "renderer.h"
#include "lib/libtetrisbrain/killfeed.h"

// Renders the board and active piece to the terminal
void drawBoard(GameState *state)
{
    printf("\e[1;1H"); // Move cursor to top left to prevent flickering

    printf("===      TETRIS     ===\n");
    printf("                       \n");

    // For ghost piece
    int ghostY = state->current.y;
    while (isValidPos(state, state->current.type, state->current.rot, state->current.x, ghostY + 1))
    {
        ghostY++;
    }

    for (int y = 0; y < BOARD_HEIGHT; y++)
    {
        // Draw the Left Wall
        printf("<|>");

        for (int x = 0; x < BOARD_WIDTH; x++)
        {
            // Check if the active piece is hovering over this exact (X, Y)
            bool isActivePieceHere = false;
            bool isGhostPieceHere = false;

            // Check if we are in the piece's 4x4 gridspace
            if (x >= state->current.x && x < state->current.x + 4 &&
                y >= state->current.y && y < state->current.y + 4)
            {
                // Translate the global board coordinate back to a local coordinates
                int px = x - state->current.x;
                int py = y - state->current.y;

                // Check our Tetris array
                int cellIndex = getRotationIndex(px, py, state->current.rot);
                int shapeIndex = state->current.type - 1;

                if (tetrominoes[shapeIndex][cellIndex] != 0)
                {
                    isActivePieceHere = true; // Solid block found
                }
            }

            // Ghost piece
            if (x >= state->current.x && x < state->current.x + 4 &&
                y >= ghostY && y < ghostY + 4)
            {
                int px = x - state->current.x;
                int py = y - ghostY;
                if (tetrominoes[state->current.type - 1][getRotationIndex(px, py, state->current.rot)] != 0)
                {
                    isGhostPieceHere = true;
                }
            }

            if (isActivePieceHere)
            {
                printf("[]"); // Top Layer: Active falling piece
            }
            else if (state->board.cells[y][x] == 8)
            {
                printf("><");
            }
            else if (state->board.cells[y][x] != 0)
            {
                printf("##"); // Middle Layer: Locked blocks
            }
            else if (isGhostPieceHere) // Ghost Piece
            {
                printf("::");
            }
            else
            {
                printf(" ."); // Background: Empty space
            }
        }

        // Draw the right wall
        printf("<|> ");
        if (BOARD_HEIGHT - y <= state->pending_garbage)
        {
            printf(" \e[0;31m#\e[0m ");
        }
        printf(" <|>");

        // For hold box to the right of the board
        if (y == 0)
        {
            printf("  HOLD BOX");
        }
        else if (y >= 1 && y <= 4)
        {
            printf("  "); // Padding
            int hy = y - 1;
            for (int hx = 0; hx < 4; hx++)
            {
                if (state->held_type != 0)
                {
                    if (tetrominoes[state->held_type - 1][getRotationIndex(hx, hy, 0)] != 0)
                        printf("[]");
                    else
                        printf("  ");
                }
                else
                    printf("  "); // Empty space if nothing held
            }
        }
        else if (y == 5)
        {
            printf("  NEXT");
        }
        else if (y >= 6 && y <= 9) // Rows 6-9: Next Piece #1
        {
            printf(" ");
            int ny = y - 6;
            int next_piece_1 = state->bag[state->bag_index]; // Point to the next piece about to spawn
            for (int hx = 0; hx < 4; hx++)
            {
                if (tetrominoes[next_piece_1 - 1][getRotationIndex(hx, ny, 0)] != 0)
                    printf("[]");
                else
                    printf("  ");
            }
        }
        else if (y >= 11 && y <= 14) // Rows 11-14: Next Piece #2
        {
            printf("  ");
            int ny = y - 11;
            int next_piece_2 = state->bag[(state->bag_index + 1) % 14]; // Point to the next two pieces about to spawn
            for (int hx = 0; hx < 4; hx++)
            {
                if (tetrominoes[next_piece_2 - 1][getRotationIndex(hx, ny, 0)] != 0)
                    printf("[]");
                else
                    printf("  ");
            }
        }
        else if (y >= 16 && y <= 19) // Rows 16-19: Next Piece #3
        {
            printf("  ");
            int ny = y - 16;
            int next_piece_3 = state->bag[(state->bag_index + 2) % 14]; // Point to the next three pieces about to spawn
            for (int hx = 0; hx < 4; hx++)
            {
                if (tetrominoes[next_piece_3 - 1][getRotationIndex(hx, ny, 0)] != 0)
                    printf("[]");
                else
                    printf("  ");
            }
        }
        else
        {
            printf("      "); // Blank spaces for row gaps
        }
        printf("\n"); // Push to next row
    }

    // Dashboard
    // Convert targeting enum back to clear text strings
    const char *mode_strings[] = {"MANUAL", "RANDOM", "KILL-OUT"};
    printf("----------------------\n");
    printf("Score: %d  |  Lines: %d\n | T-Spins: %d\n", state->score, state->lines_cleared, state->t_spins);
    printf("   [Left | Right] Move\n  [Down] Soft Drop\n  [Up | X] Rotate CW\n  [Z] Rotate CCW\n   [T] Change Targeting Mode\n   [R] Swap Targets\n   [Space] Hard Drop\n  [H] Hold\n  [Q] Quit\n");
    printf(" Target: P%d (%s)\n", state->target_player_id, mode_strings[state->target_mode]);
    drawKillFeed();
    fflush(stdout); // Force print
}