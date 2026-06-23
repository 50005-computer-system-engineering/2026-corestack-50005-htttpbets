#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>  // POSIX OS API
#include <ncurses.h> // Colours !!
#include <time.h>    // Randomize piece spawn
#include "tetrisu.h"

/* ----- NCURSES ENVIRONMENT ----- */
void setup()
{
    initscr();             // Initialize
    cbreak();              // Disable line buffering (ignore enter key)
    noecho();              // Do not print typed charas to the screen
    curs_set(0);           // Hide cursor
    keypad(stdscr, TRUE);  // Enable capture of arrow keys
    srand(time(NULL));     // Set new seed to ensure every initial spawn piece for each game is different
    nodelay(stdscr, TRUE); // Make getch() non-blocking; returns ERR if no key detected

    // Time for colour!
    if (has_colors())
    {
        start_color();
        // Colour pairs defined for each piece + background
        init_pair(1, COLOR_BLACK, COLOR_CYAN);    // I piece
        init_pair(2, COLOR_BLACK, COLOR_YELLOW);  // O piece
        init_pair(3, COLOR_BLACK, COLOR_MAGENTA); // T piece
        init_pair(4, COLOR_BLACK, COLOR_GREEN);   // S piece
        init_pair(5, COLOR_BLACK, COLOR_RED);     // Z piece
        init_pair(6, COLOR_BLACK, COLOR_BLUE);    // J piece
        init_pair(7, COLOR_BLACK, COLOR_WHITE);   // L piece
        // Following default tetris colours (ncurses doesn't support orange :( ))
    }
}

// Renders the board and active piece to the terminal
void drawBoard(GameState *state)
{
    erase(); // In-built NCURSES function to clear the virtual screen memory

    // Move to (0,0) and print text
    mvprintw(0, 0, "   === TETRIS ===   ");

    for (int y = 0; y < BOARD_HEIGHT; y++)
    {
        mvprintw(y + 2, 0, "|"); // Draw the left wall; (0, y+2) so is below the header

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
            // Drawing with colours!
            if (isActivePieceHere)
            {
                // Set colour pair that matches piece type and print
                attron(COLOR_PAIR(state->current.type));
                printw("  ");
                attroff(COLOR_PAIR(state->current.type));
            }
            else if (state->board.cells[y][x] != 0)
            {
                // Same thing here
                int lockedType = state->board.cells[y][x];
                attron(COLOR_PAIR(lockedType));
                printw("  ");
                attroff(COLOR_PAIR(lockedType));
            }
            else
            {
                printw(" ."); // Background: Empty space; No colour
            }
        }
        printw("|"); // Draw the right wall
    }

    // Draw the floor with stats
    mvprintw(BOARD_HEIGHT + 2, 0, "<!>================<!>");
    mvprintw(BOARD_HEIGHT + 3, 0, "   Score: %-6d Lines: %d", state->score, state->lines_cleared);
    mvprintw(BOARD_HEIGHT + 4, 0, "   Controls: [Left | Right] Move  [Down] Soft Drop  [Up] Rotate  [Space] Hard Drop  [Q] Quit");

    // Force push
    refresh();
}

/* --- MAIN GAME LOOP --- */
int main()
{
    // Spawn a game state
    GameState myGame;

    // Set up the terminal for the game
    setup();
    startGame(&myGame);

    // Timing set up => make the game more playable
    int gravityTimer = 0;
    int gravityThreshold = 25; // How many loop cycles before the piece drops 1 row

    // --- THE GAME LOOP ---
    while (!myGame.game_over)
    {
        // Read user inputs
        int key = getch();

        if (key != ERR) // ERR means no key was pressed
        {
            // keypad(stdscr, TRUE) translates raw Linux bytes into macros, so no need process directly
            switch (key)
            {
            case KEY_LEFT:
                if (isValidPos(&myGame, myGame.current.type, myGame.current.rot, myGame.current.x - 1, myGame.current.y))
                {
                    myGame.current.x--;
                }
                break;
            case KEY_RIGHT:
                if (isValidPos(&myGame, myGame.current.type, myGame.current.rot, myGame.current.x + 1, myGame.current.y))
                {
                    myGame.current.x++;
                }
                break;
            case KEY_DOWN: // Soft Drop
                if (isValidPos(&myGame, myGame.current.type, myGame.current.rot, myGame.current.x, myGame.current.y + 1))
                {
                    // If the piece moved successfully
                    myGame.current.y++;
                    gravityTimer = 0; // Prevention for double-drop stutter
                }
                break;
            case KEY_UP: // Rotate
                rotateCurrentPiece(&myGame);
                break;
            case ' ': // Hard Drop
                while (isValidPos(&myGame, myGame.current.type, myGame.current.rot, myGame.current.x, myGame.current.y + 1))
                {
                    myGame.current.y++;
                }
                // Lock immediately
                tickGame(&myGame);
                gravityTimer = 0; // Reset Timer
                flushinp();       // Clear kb buffer to prevent misfire
                break;
            case 'q':
            case 'Q':
                myGame.game_over = 1; // Trigger game over sequence
                break;
            }
        }

        // Gravity
        gravityTimer++;
        if (gravityTimer >= gravityThreshold)
        {
            tickGame(&myGame);
            gravityTimer = 0; // Reset the timer
        }

        // Render the board
        drawBoard(&myGame);

        // Delay frames to be visible to the human eye
        usleep(10000);
    }

    // Clean up after game ends
    drawBoard(&myGame); // Show final state

    // Game Over Banner
    int startY = BOARD_HEIGHT + 6;
    mvprintw(startY, 0, "<!> ====================== <!>");
    attron(COLOR_PAIR(5)); // RED RED
    mvprintw(startY + 1, 0, "<!>       GAME OVER!       <!>");
    attroff(COLOR_PAIR(5)); // Turn off color to show white text

    mvprintw(startY + 2, 0, "<!>      Final Score: %-4d <!>", myGame.score);
    mvprintw(startY + 3, 0, "<!>    Lines Cleared: %-4d <!>", myGame.lines_cleared);
    mvprintw(startY + 4, 0, "<!> ====================== <!>");

    // Blinks the prompt so the user knows they can exit
    attron(A_BLINK);
    mvprintw(startY + 6, 0, "Press any key to exit to terminal...");
    attroff(A_BLINK);

    // Push changes to the main screen
    refresh();

    // Turn blocking back ON
    nodelay(stdscr, FALSE);

    // Wait for input stroke
    getch();

    // Destroy and return to normal terminal
    endwin();

    return 0;
}