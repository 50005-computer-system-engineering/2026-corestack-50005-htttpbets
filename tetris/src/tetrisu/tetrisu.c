#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <termios.h> // Linux terminal control
#include <unistd.h>  // POSIX OS API
#include <fcntl.h>   // File control (for non-blocking reads)
#include "tetrisu.h"

// --- TERMINAL CONTROL (termios) ---
// Linux in-built terminal flags
struct termios orig_termios;

// Reset terminal back to normal; if not will remain broken
void disableRawMode(void)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios); // Apply stdin NOW, then reset back to handed saved state
    printf("\e[?25h");                               // Show cursor
    fflush(stdout);                                  // Force print
}

// Invoke terminal settings
void enableRawMode()
{
    tcgetattr(STDIN_FILENO, &orig_termios); // Save current terminal settings and store as saved state
    atexit(disableRawMode);                 // Enforces revert back to normal terminal settings when user quits / ctrl-c
    struct termios raw = orig_termios;      // Make a copy of current state for us to work on

    // ICANON => system holds input inside a buffer until enter key is pressed
    // When disabled, sends every single keypress directly to the application instantly
    // ECHO => shows current input on screen; when disabled, input is completely hidden from screen
    raw.c_lflag &= ~(ECHO | ICANON); // Turn off "print keys to screen" and "wait for enter key"

    tcsetattr(STDIN_FILENO, TCSANOW, &raw); // Apply modifications on this saved state copy
    printf("\e[?25l");                      // Hide blinking cursor
    fflush(stdout);                         // Force print
}

// Checks if a key has been pressed (Non-blocking)
int kbhit(void)
{
    int ch;

    // In Linux, input is also treated as a file;
    int oldf = fcntl(STDIN_FILENO, F_GETFL, 0);      // Get current stdin
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK); // Add special flag => if no input registered, instantly return EOF and continue
    ch = getchar();                                  // Attempt to read a chara
    fcntl(STDIN_FILENO, F_SETFL, oldf);              // Restore normal behaviour

    // If a key was pressed
    if (ch != EOF)
    {
        ungetc(ch, stdin); // Feed obtained input into actual input
        return 1;          // Return true
    }

    return 0; // Return false
}

// Wrapper to grab the character (input) from stdin once we know it's there
int getch(void)
{
    return getchar();
}

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
    fflush(stdout); // Force print
}

// --- MAIN GAME LOOP ---
int main()
{
    // Spawn a game state
    GameState myGame;

    // Clear terminal screen
    // Set up the terminal for the game
    enableRawMode();
    printf("\e[1;1H\e[2J");
    fflush(stdout);

    startGame(&myGame);
    myGame.held_type = 0; // Initialize hold box
    myGame.has_held = false;

    // Timing set up => make the game more playable
    int gravityTimer = 0;
    int gravityThreshold = 50; // How many loop cycles before the piece drops 1 row
    int lockTimer = 0;
    int lockThreshold = 50;

    // --- THE GAME LOOP ---
    while (!myGame.game_over)
    {
        // Track current gravity of piece for lock delay
        int current_gravity = gravityThreshold - ((myGame.level - 1) * 5);
        if (current_gravity < 5)
        {
            current_gravity = 5;
        }

        // Read user inputs
        if (kbhit())
        {
            // Call getch() wrapper
            int key = getch();

            // Linux arrow keys send 3 characters instantly => escape (27), '[', and a letter
            if (key == 27) // Escape or arrow key?
            {
                if (kbhit() && getch() == '[') // Bracket right after?
                {
                    if (kbhit()) // Decide output based on letter
                    {
                        switch (getch())
                        {
                        case 'A': // Up arrow (rotate clockwise)
                            rotateCurrentPiece(&myGame);
                            lockTimer = 0;
                            break;
                        case 'D': // Left arrow
                            if (isValidPos(&myGame, myGame.current.type, myGame.current.rot, myGame.current.x - 1, myGame.current.y))
                            {
                                myGame.current.x--;
                                myGame.last_action_rotation = false;
                                lockTimer = 0;
                            }
                            break;
                        case 'C': // Right arrow
                            if (isValidPos(&myGame, myGame.current.type, myGame.current.rot, myGame.current.x + 1, myGame.current.y))
                            {
                                myGame.current.x++;
                                myGame.last_action_rotation = false;
                                lockTimer = 0;
                            }
                            break;
                        case 'B': // Down arrow (soft drop + lock delay)
                            if (isValidPos(&myGame, myGame.current.type, myGame.current.rot, myGame.current.x, myGame.current.y + 1))
                            {
                                myGame.current.y++;
                                myGame.last_action_rotation = false;
                                gravityTimer = 0; // Reset timer to prevent double dropping
                            }
                            break;
                        }
                    }
                }
            }
            else if (key == 'x' || key == 'X') // Rotate clockwise (alternate key)
            {
                rotateCurrentPiece(&myGame);
                lockTimer = 0;
            }
            else if (key == 'z' || key == 'Z') // Rotate counterclockwise
            {
                rotateCounterClockwise(&myGame);
                lockTimer = 0;
            }
            else if (key == ' ') // Spacebar (hard drop)
            {
                while (isValidPos(&myGame, myGame.current.type, myGame.current.rot, myGame.current.x, myGame.current.y + 1))
                {
                    myGame.current.y++;
                    myGame.last_action_rotation = false;
                }
                tickGame(&myGame);
                gravityTimer = 0;

                // Clear the raw terminal input buffer to prevent double drops
                tcflush(STDIN_FILENO, TCIFLUSH);
            }
            else if (key == 'h' || key == 'H') // H to hold
            {
                if (!myGame.has_held)
                {
                    holdPiece(&myGame);
                    gravityTimer = 0;
                }
                tcflush(STDIN_FILENO, TCIFLUSH);
            }
            else if (key == 'q' || key == 'Q') // Q to quit
            {
                break; // Exit
            }
        }

        // Gravity + Lock Delay
        bool is_resting = !isValidPos(&myGame, myGame.current.type, myGame.current.rot, myGame.current.x, myGame.current.y + 1);
        if (is_resting)
        {
            // Lock Timer
            lockTimer++;
            if (lockTimer >= lockThreshold)
            {
                tickGame(&myGame); // Lock piece
                // Reset env variables
                lockTimer = 0;
                gravityTimer = 0;
            }
        }
        else
        {
            // Gravity Timer
            lockTimer = 0;
            gravityTimer++;
            if (gravityTimer >= current_gravity)
            {
                tickGame(&myGame);
                gravityTimer = 0;
            }
        }

        // Render the board
        drawBoard(&myGame);

        // Delay frames to be visible to the human eye
        usleep(10000);
    }

    // Clean up after game ends
    drawBoard(&myGame);
    printf("\n\n");
    printf("<!> ====================== <!>\n");
    printf("<!>       GAME OVER!       <!>\n");
    printf("<!>      Final Score: %-4d <!>\n", myGame.score);
    printf("<!>     Lines Cleared: %-4d<!>\n", myGame.lines_cleared);
    printf("<!> ====================== <!>\n");
    printf("\nPress any key to exit...\n");

    // Only quit when a keystroke is detected.
    tcflush(STDIN_FILENO, TCIFLUSH);
    while (!kbhit())
    {
        usleep(10000);
    }
    getch();

    return 0;
}