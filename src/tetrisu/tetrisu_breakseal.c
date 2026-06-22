#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <termios.h> // Linux terminal control
#include <unistd.h>  // POSIX OS API
#include <fcntl.h>   // File control (for non-blocking reads)
#include "tetrisbrain.h"

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

    for (int y = 0; y < BOARD_HEIGHT; y++)
    {
        printf("|"); // Draw the left wall

        for (int x = 0; x < BOARD_WIDTH; x++)
        {
            // Check if the active piece is hovering over this exact (X, Y)
            bool isActivePieceHere = false;

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

            if (isActivePieceHere)
            {
                printf("[]"); // Top Layer: Active falling piece
            }
            else if (state->board.cells[y][x] != 0)
            {
                printf("##"); // Middle Layer: Locked blocks
            }
            else
            {
                printf(" ."); // Background: Empty space
            }
        }
        printf("|\n"); // Draw the right wall
    }

    // Draw the floor
    printf("----------------------\n");
    printf("Score: %d  |  Lines: %d\n", state->score, state->lines_cleared);
    printf("Controls:\n  [Arrows] Move/Drop\n  [Space] Rotate\n  [Q] Quit\n");
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

    // Timing set up => make the game more playable
    int gravityTimer = 0;
    int gravityThreshold = 25; // How many loop cycles before the piece drops 1 row

    // --- THE GAME LOOP ---
    while (!myGame.game_over)
    {
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
                        case 'D': // Left arrow
                            if (isValidPos(&myGame, myGame.current.type, myGame.current.rot, myGame.current.x - 1, myGame.current.y))
                                myGame.current.x--;
                            break;
                        case 'C': // Right arrow
                            if (isValidPos(&myGame, myGame.current.type, myGame.current.rot, myGame.current.x + 1, myGame.current.y))
                                myGame.current.x++;
                            break;
                        case 'B': // Down arrow
                            if (isValidPos(&myGame, myGame.current.type, myGame.current.rot, myGame.current.x, myGame.current.y + 1))
                                myGame.current.y++;
                            gravityTimer = 0; // Reset timer to prevent double dropping
                            break;
                        }
                    }
                }
            }
            else if (key == ' ') // Spacebar (Default rotate key)
            {
                rotateCurrentPiece(&myGame);
            }
            else if (key == 'q' || key == 'Q') // Q to quit
            {
                break; // Exit
            }
        }

        // Gravity
        gravityTimer++;
        if (gravityTimer >= gravityThreshold)
        {
            tickGame(&myGame);
            gravityTimer = 0;  // Reset the timer
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
    while (!kbhit()) 
    {
        usleep(10000); 
    }
    getch();

    return 0;
}