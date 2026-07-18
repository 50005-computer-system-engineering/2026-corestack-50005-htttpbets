#include <stdio.h>
#include <stdbool.h>
#include "lib/libtetrisbrain/board_control.h"
#include "renderer.h"
#include "lib/libtetrisbrain/killfeed.h"

// Renders the board and active piece to the terminal
// 1 board for each player => only done for testing purposes
void drawBothBoards(GameState *p1, GameState *p2)
{
    printf("\e[1;1H"); // Move cursor to top left to prevent flickering

    // Header
    printf("      === PLAYER 1 ===                             === PLAYER 2 ===   \n");
    printf("                                                                   \n");

    // --- GHOST PIECE MATH FOR BOTH PLAYERS ---
    int p1_ghostY = p1->current.y;
    while (isValidPos(p1, p1->current.type, p1->current.rot, p1->current.x, p1_ghostY + 1))
    {
        p1_ghostY++;
    }

    int p2_ghostY = p2->current.y;
    while (isValidPos(p2, p2->current.type, p2->current.rot, p2->current.x, p2_ghostY + 1))
    {
        p2_ghostY++;
    }

    for (int y = 0; y < BOARD_HEIGHT; y++)
    {
        // P1 Half
        // P1 Left Wall
        printf("<|>");

        for (int x = 0; x < BOARD_WIDTH; x++)
        {
            // Check if the active piece is hovering over this exact (X, Y)
            bool isActivePieceHere = false;
            bool isGhostPieceHere = false;

            // P1 Active Piece
            if (x >= p1->current.x && x < p1->current.x + 4 && y >= p1->current.y && y < p1->current.y + 4)
            {
                // Translate the global board coordinate back to a local coordinates
                int px = x - p1->current.x;
                int py = y - p1->current.y;
                // Check our Tetris array
                int cellIndex = getRotationIndex(px, py, p1->current.rot);
                int shapeIndex = p1->current.type - 1;

                if (tetrominoes[shapeIndex][cellIndex] != 0)
                {
                    isActivePieceHere = true; // Solid block found
                }
            }
            // P1 Ghost Piece
            if (x >= p1->current.x && x < p1->current.x + 4 && y >= p1_ghostY && y < p1_ghostY + 4)
            {
                int px = x - p1->current.x;
                int py = y - p1_ghostY;
                if (tetrominoes[p1->current.type - 1][getRotationIndex(px, py, p1->current.rot)] != 0)
                {
                    isGhostPieceHere = true;
                }
            }

            // P1 Draw Priorities
            if (isActivePieceHere)
            {
                printf("[]"); // Top Layer: Active falling piece
            }
            else if (p1->board.cells[y][x] == 8)
            {
                printf("><");
            }
            else if (p1->board.cells[y][x] != 0)
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
        // P1 Right Wall
        printf("<|> ");
        if (BOARD_HEIGHT - y <= p1->pending_garbage)
        {
            printf(" \e[0;31m#\e[0m ");
        }
        printf(" <|>");

        // P1 Hold Box (STRICT 10 CHARACTER WIDTH)
        if (y == 0)
        {
            printf("   HOLD   ");
        }
        else if (y >= 1 && y <= 4)
        {
            printf(" "); // 1 space padding
            int hy = y - 1;
            for (int hx = 0; hx < 4; hx++)
            {
                if (p1->held_type != 0)
                {
                    if (tetrominoes[p1->held_type - 1][getRotationIndex(hx, hy, 0)] != 0)
                        printf("[]");
                    else
                        printf("  ");
                }
                else
                    printf("  "); // Empty space if nothing held
            }
            printf(" "); // 1 space padding
        }
        else if (y == 5)
        {
            printf("   NEXT   ");
        }
        else if (y >= 6 && y <= 9) // Rows 6-9: Next Piece #1
        {
            printf("  ");
            int ny = y - 6;
            int next_piece_1 = p1->bag[p1->bag_index]; // Point to the next piece about to spawn
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
            int next_piece_2 = p1->bag[(p1->bag_index + 1) % 14]; // Point to the next two pieces about to spawn
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
            int next_piece_3 = p1->bag[(p1->bag_index + 2) % 14]; // Point to the next three pieces about to spawn
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
            printf("          "); // 10 spaces of empty padding to keep P2 aligned!
        }

        printf("  |  "); // Middle Divider

        // P2 Half
        // P2 Left Wall
        printf("<|>");

        for (int x = 0; x < BOARD_WIDTH; x++)
        {
            // Check if the active piece is hovering over this exact (X, Y)
            bool isActivePieceHere = false;
            bool isGhostPieceHere = false;

            // P2 Active Piece
            if (x >= p2->current.x && x < p2->current.x + 4 && y >= p2->current.y && y < p2->current.y + 4)
            {
                // Translate the global board coordinate back to a local coordinates
                int px = x - p2->current.x;
                int py = y - p2->current.y;
                // Check our Tetris array
                int cellIndex = getRotationIndex(px, py, p2->current.rot);
                int shapeIndex = p2->current.type - 1;

                if (tetrominoes[shapeIndex][cellIndex] != 0)
                {
                    isActivePieceHere = true; // Solid block found
                }
            }
            // P2 Ghost Piece
            if (x >= p2->current.x && x < p2->current.x + 4 && y >= p2_ghostY && y < p2_ghostY + 4)
            {
                int px = x - p2->current.x;
                int py = y - p2_ghostY;
                if (tetrominoes[p2->current.type - 1][getRotationIndex(px, py, p2->current.rot)] != 0)
                {
                    isGhostPieceHere = true;
                }
            }

            if (isActivePieceHere)
            {
                printf("[]"); // Top Layer: Active falling piece
            }
            else if (p2->board.cells[y][x] == 8)
            {
                printf("><");
            }
            else if (p2->board.cells[y][x] != 0)
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
        // P2 Right Wall
        printf("<|> ");
        if (BOARD_HEIGHT - y <= p2->pending_garbage)
        {
            printf(" \e[0;31m#\e[0m ");
        }
        printf(" <|>");

        // P2 Hold Box
        if (y == 0)
        {
            printf("   HOLD   ");
        }
        else if (y >= 1 && y <= 4)
        {
            printf(" "); // Padding
            int hy = y - 1;
            for (int hx = 0; hx < 4; hx++)
            {
                if (p2->held_type != 0)
                {
                    if (tetrominoes[p2->held_type - 1][getRotationIndex(hx, hy, 0)] != 0)
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
            printf("  ");
            int ny = y - 6;
            int next_piece_1 = p2->bag[p2->bag_index]; // Point to the next piece about to spawn
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
            int next_piece_2 = p2->bag[(p2->bag_index + 1) % 14]; // Point to the next two pieces about to spawn
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
            int next_piece_3 = p2->bag[(p2->bag_index + 2) % 14]; // Point to the next three pieces about to spawn
            for (int hx = 0; hx < 4; hx++)
            {
                if (tetrominoes[next_piece_3 - 1][getRotationIndex(hx, ny, 0)] != 0)
                    printf("[]");
                else
                    printf("  ");
            }
        }

        printf("\n"); // Finally push to next row
    }

    // DUAL DASHBOARD
    printf("<!>====================<!>                     <!>====================<!>\n");
    printf(" Lvl: %-2d  Score: %-5d                         Lvl: %-2d  Score: %-5d\n", p1->level, p1->score, p2->level, p2->score);
    printf(" Lines: %-3d   T-Spins: %-2d                      Lines: %-3d   T-Spins: %-2d\n", p1->lines_cleared, p1->t_spins, p2->lines_cleared, p2->t_spins);
    printf(" Controls: Move / Hold / Hard Drop             Controls: Move / Hold / Hard Drop\n");
    printf(" Controls: WASD / F / G                        Controls: Arrows / H / Space");

    drawKillFeed();
    fflush(stdout);
}

/*  uncomment for single player

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

    // Draw the floor
    printf("----------------------\n");
    printf("Score: %d  |  Lines: %d\n | T-Spins: %d\n", state->score, state->lines_cleared, state->t_spins);
    printf("   [Left | Right] Move\n  [Down] Soft Drop\n  [Up | X] Rotate CW\n  [Z] Rotate CCW\n   [Space] Hard Drop\n  [H] Hold\n  [Q] Quit");
    drawKillFeed();
    fflush(stdout); // Force print
}
    
*/