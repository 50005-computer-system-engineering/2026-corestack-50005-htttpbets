/* DISCLAIMER: AI USED HERE TO HELP VISUALIZE AND TEST GARBAGE LOGIC */

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
// 1 board for each player => only done for testing purposes
void drawBothBoards(GameState *p1, GameState *p2)
{
    printf("\e[1;1H"); // Move cursor to top left to prevent flickering

    // 75-character wide header
    printf("      === PLAYER 1 ===                      === PLAYER 2 ===   \n");
    printf("                                                \n");

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
        if (BOARD_HEIGHT - y <= p1->pending_garbage)
        {
            printf("<#>");
        }
        else
        {
            printf("<|>");
        }

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
        printf("|"); // P1 Right Wall

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
        else
        {
            printf("          "); // 10 spaces of empty padding to keep P2 aligned!
        }

        // P2 Half
        // P2 Left Wall
        if (BOARD_HEIGHT - y <= p2->pending_garbage)
        {
            printf("<#>");
        }
        else
        {
            printf("<|>");
        }

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
        printf("|"); // P2 Right Wall

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

        printf("\n"); // Finally push to next row
    }

    // --- 3. DUAL DASHBOARD ---
    printf("<!>====================<!>             <!>====================<!>\n");
    printf(" Lvl: %-2d  Score: %-5d                 Lvl: %-2d  Score: %-5d\n", p1->level, p1->score, p2->level, p2->score);
    printf(" Lines: %-3d   T-Spins: %-2d              Lines: %-3d   T-Spins: %-2d\n", p1->lines_cleared, p1->t_spins, p2->lines_cleared, p2->t_spins);

    // CRITICAL: Still no \n on the very last line to prevent terminal scrolling bugs!
    printf(" Controls: WASD / F / G                Controls: Arrows / H / Space");

    fflush(stdout);
}

// --- MAIN GAME LOOP ---
int main()
{
    // Spawn a game state
    GameState player1;
    GameState player2;

    // Clear terminal screen
    // Set up the terminal for the game
    enableRawMode();
    printf("\e[1;1H\e[2J");
    fflush(stdout);

    startGame(&player1);
    player1.held_type = 0; // Initialize hold box
    player1.has_held = false;

    // Timing set up => make the game more playable
    int p1_gravityTimer = 0;
    int p1_gravityThreshold = 50; // How many loop cycles before the piece drops 1 row
    int p1_lockTimer = 0;
    int p1_lockThreshold = 50;

    startGame(&player2);
    player2.held_type = 0; // Initialize hold box
    player2.has_held = false;

    // Timing set up => make the game more playable
    int p2_gravityTimer = 0;
    int p2_gravityThreshold = 50; // How many loop cycles before the piece drops 1 row
    int p2_lockTimer = 0;
    int p2_lockThreshold = 50;

    // --- THE GAME LOOP ---
    while (!player1.game_over && !player2.game_over)
    {
        // Track current gravity of piece for lock delay
        int p1_current_gravity = p1_gravityThreshold - ((player1.level - 1) * 5);
        if (p1_current_gravity < 5)
        {
            p1_current_gravity = 5;
        }

        int p2_current_gravity = p2_gravityThreshold - ((player2.level - 1) * 5);
        if (p2_current_gravity < 5)
        {
            p2_current_gravity = 5;
        }

        // Read user inputs
        while (kbhit())
        {
            // Call getch() wrapper
            int key = getch();

            // Player 2 keybinds
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
                            rotateCurrentPiece(&player2);
                            p2_lockTimer = 0;
                            break;
                        case 'D': // Left arrow
                            if (isValidPos(&player2, player2.current.type, player2.current.rot, player2.current.x - 1, player2.current.y))
                            {
                                player2.current.x--;
                                player2.last_action_rotation = false;
                                p2_lockTimer = 0;
                            }
                            break;
                        case 'C': // Right arrow
                            if (isValidPos(&player2, player2.current.type, player2.current.rot, player2.current.x + 1, player2.current.y))
                            {
                                player2.current.x++;
                                player2.last_action_rotation = false;
                                p2_lockTimer = 0;
                            }
                            break;
                        case 'B': // Down arrow (soft drop + lock delay)
                            if (isValidPos(&player2, player2.current.type, player2.current.rot, player2.current.x, player2.current.y + 1))
                            {
                                player2.current.y++;
                                player2.last_action_rotation = false;
                                p2_gravityTimer = 0; // Reset timer to prevent double dropping
                            }
                            break;
                        }
                    }
                }
            }
            else if (key == 'x' || key == 'X') // Rotate clockwise (alternate key)
            {
                rotateCurrentPiece(&player2);
                p2_lockTimer = 0;
            }
            else if (key == 'z' || key == 'Z') // Rotate counterclockwise
            {
                rotateCounterClockwise(&player2);
                p2_lockTimer = 0;
            }
            else if (key == ' ') // Spacebar (hard drop)
            {
                while (isValidPos(&player2, player2.current.type, player2.current.rot, player2.current.x, player2.current.y + 1))
                {
                    player2.current.y++;
                    player2.last_action_rotation = false;
                }

                // Calculate Garbage and send
                tickGame(&player2);
                if (player2.outgoing_garbage > 0)
                {
                    player1.pending_garbage += player2.outgoing_garbage;
                    player2.outgoing_garbage = 0; // Reset after sending
                }
                p2_gravityTimer = 0;
            }
            else if (key == 'h' || key == 'H') // H to hold
            {
                if (!player2.has_held)
                {
                    holdPiece(&player2);
                    p2_gravityTimer = 0;
                }
            }

            // --- PLAYER 1 (WASD) Controls ---
            else if (key == 'w' || key == 'W') // P1 Rotate CW
            {
                rotateCurrentPiece(&player1);
                p1_lockTimer = 0;
            }
            else if (key == 'a' || key == 'A') // P1 Left
            {
                if (isValidPos(&player1, player1.current.type, player1.current.rot, player1.current.x - 1, player1.current.y))
                {
                    player1.current.x--;
                    player1.last_action_rotation = false;
                    p1_lockTimer = 0;
                }
            }
            else if (key == 'd' || key == 'D') // P1 Right
            {
                if (isValidPos(&player1, player1.current.type, player1.current.rot, player1.current.x + 1, player1.current.y))
                {
                    player1.current.x++;
                    player1.last_action_rotation = false;
                    p1_lockTimer = 0;
                }
            }
            else if (key == 's' || key == 'S') // P1 Down (Soft Drop)
            {
                if (isValidPos(&player1, player1.current.type, player1.current.rot, player1.current.x, player1.current.y + 1))
                {
                    player1.current.y++;
                    player1.last_action_rotation = false;
                    p1_gravityTimer = 0;
                }
            }
            else if (key == 'g' || key == 'G') // P1 Hard Drop
            {
                while (isValidPos(&player1, player1.current.type, player1.current.rot, player1.current.x, player1.current.y + 1))
                {
                    player1.current.y++;
                    player1.last_action_rotation = false;
                }

                // Calculate Garbage and send
                tickGame(&player1);
                if (player1.outgoing_garbage > 0)
                {
                    player2.pending_garbage += player1.outgoing_garbage;
                    player1.outgoing_garbage = 0; // Reset after sending
                }

                p1_gravityTimer = 0;
            }
            else if (key == 'f' || key == 'F') // P1 Hold
            {
                if (!player1.has_held)
                {
                    holdPiece(&player1);
                    p1_gravityTimer = 0;
                }
            }
            else if (key == 'q' || key == 'Q') // Q to quit
            {
                // Set game over states for both P1 and P2
                player1.game_over = true;
                player2.game_over = true;
                break; // Exit
            }
        }

        // Gravity + Lock Delay
        bool p1_resting = !isValidPos(&player1, player1.current.type, player1.current.rot, player1.current.x, player1.current.y + 1);
        if (p1_resting)
        {
            // Lock Timer
            p1_lockTimer++;
            if (p1_lockTimer >= p1_lockThreshold)
            {
                // Calculate Garbage and send
                tickGame(&player1);
                if (player1.outgoing_garbage > 0)
                {
                    player2.pending_garbage += player1.outgoing_garbage;
                    player1.outgoing_garbage = 0; // Reset after sending
                }

                // Reset env variables
                p1_lockTimer = 0;
                p1_gravityTimer = 0;
            }
        }
        else
        {
            // Gravity Timer
            p1_lockTimer = 0;
            p1_gravityTimer++;
            if (p1_gravityTimer >= p1_current_gravity)
            {
                tickGame(&player1);
                p1_gravityTimer = 0;
            }
        }

        // For player 2
        bool p2_resting = !isValidPos(&player2, player2.current.type, player2.current.rot, player2.current.x, player2.current.y + 1);
        if (p2_resting)
        {
            // Lock Timer
            p2_lockTimer++;
            if (p2_lockTimer >= p2_lockThreshold)
            {
                // Calculate Garbage and send
                tickGame(&player2);
                if (player2.outgoing_garbage > 0)
                {
                    player1.pending_garbage += player2.outgoing_garbage;
                    player2.outgoing_garbage = 0; // Reset after sending
                }
                // Reset env variables
                p2_lockTimer = 0;
                p2_gravityTimer = 0;
            }
        }
        else
        {
            // Gravity Timer
            p2_lockTimer = 0;
            p2_gravityTimer++;
            if (p2_gravityTimer >= p2_current_gravity)
            {
                tickGame(&player2);
                p2_gravityTimer = 0;
            }
        }

        // Render the boards
        drawBothBoards(&player1, &player2);

        // Delay frames to be visible to the human eye
        usleep(10000);
    }

    // Clean up after game ends
    drawBothBoards(&player1, &player2);
    printf("\n\n");
    printf("<!> ====================== <!>\n");

    if (player1.game_over && !player2.game_over)
    {
        printf("<!>    PLAYER 2 WINS!      <!>\n");
    }
    else if (player2.game_over && !player1.game_over)
    {
        printf("<!>    PLAYER 1 WINS!      <!>\n");
    }
    else
    {
        printf("<!>         DRAW!          <!>\n");
    }
    printf("<!>       GAME OVER!       <!>\n");
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