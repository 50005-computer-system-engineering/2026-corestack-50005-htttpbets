#include <ncurses.h>
#include <unistd.h>
#include <locale.h>
#include "tetrisbrain.h"

// --- NCURSES SETUP ---
void setupNcurses()
{
    initscr();             // Initialize the ncurses window
    cbreak();              // Disable line buffering (no waiting for Enter)
    noecho();              // Don't print typed characters to the screen
    keypad(stdscr, TRUE);  // Enable capture of special keys (like Arrows)
    nodelay(stdscr, TRUE); // Make getch() non-blocking (returns ERR if no key)
    curs_set(0);           // Hide the blinking cursor

    // --- COLOR SETUP ---
    if (has_colors())
    {
        start_color();
        use_default_colors(); // Tells ncurses to respect your terminal's background

        // Use -1 instead of COLOR_BLACK for a transparent background
        init_pair(1, COLOR_CYAN, -1);
        init_pair(2, COLOR_YELLOW, -1);
        init_pair(3, COLOR_MAGENTA, -1);
        init_pair(4, COLOR_GREEN, -1);
        init_pair(5, COLOR_RED, -1);
        init_pair(6, COLOR_BLUE, -1);
        init_pair(7, COLOR_WHITE, -1);
    }
}

// --- RENDERER ---
void drawBoard(GameState *state)
{
    erase(); // Ncurses function to clear the virtual screen memory

    // mvprintw(Y, X, string) moves the cursor and prints in one step
    mvprintw(0, 0, "   === NCURSES TETRIS ===   ");

    for (int y = 0; y < BOARD_HEIGHT; y++)
    {
        // Add +2 to Y so we draw below the title header
        mvprintw(y + 2, 0, "<!>");

        for (int x = 0; x < BOARD_WIDTH; x++)
        {
            bool isActivePieceHere = false;

            if (x >= state->current.x && x < state->current.x + 4 &&
                y >= state->current.y && y < state->current.y + 4)
            {
                int px = x - state->current.x;
                int py = y - state->current.y;
                int cellIndex = getRotationIndex(px, py, state->current.rot);
                int shapeIndex = state->current.type - 1;

                if (tetrominoes[shapeIndex][cellIndex] != 0)
                {
                    isActivePieceHere = true;
                }
            }

            // --- DRAW WITH COLORS ---
            // --- DRAW WITH THE REVERSE SPACE TRICK ---
            if (isActivePieceHere)
            {
                // Turn on the color AND the reverse attribute
                attron(COLOR_PAIR(state->current.type) | A_REVERSE);
                printw("  "); // Print two blank spaces!
                attroff(COLOR_PAIR(state->current.type) | A_REVERSE);
            }
            else if (state->board.cells[y][x] != 0)
            {
                int lockedType = state->board.cells[y][x];
                attron(COLOR_PAIR(lockedType) | A_REVERSE);
                printw("  "); // Print two blank spaces!
                attroff(COLOR_PAIR(lockedType) | A_REVERSE);
            }
            else
            {
                printw(" ."); // Keep the empty space normal
            }
        }
        printw("<!>"); // Right wall
    }

    // Floor and Stats
    mvprintw(BOARD_HEIGHT + 2, 0, "<!>====================<!>");
    mvprintw(BOARD_HEIGHT + 3, 0, "   Score: %-6d Lines: %d", state->score, state->lines_cleared);
    mvprintw(BOARD_HEIGHT + 4, 0, "   Controls: [Arrows] Move/Drop  [Space] Rotate  [Q] Quit");

    refresh(); // Ncurses pushes everything from virtual memory to the real screen instantly!
}

// --- MAIN GAME LOOP ---
int main()
{
    GameState myGame;

    // <--- ADD THIS LINE FIRST --->
    setlocale(LC_ALL, ""); // Tells C and ncurses to support UTF-8 characters like ██

    setupNcurses();
    startGame(&myGame);

    int gravityTimer = 0;
    int gravityThreshold = 50;

    while (!myGame.game_over)
    {
        // 1. Process Inputs (So much cleaner now!)
        int key = getch();

        if (key != ERR) // ERR means no key was pressed
        {
            switch (key)
            {
            case KEY_LEFT:
                if (isValidPos(&myGame, myGame.current.type, myGame.current.rot, myGame.current.x - 1, myGame.current.y))
                    myGame.current.x--;
                break;
            case KEY_RIGHT:
                if (isValidPos(&myGame, myGame.current.type, myGame.current.rot, myGame.current.x + 1, myGame.current.y))
                    myGame.current.x++;
                break;
            case KEY_DOWN:
                if (isValidPos(&myGame, myGame.current.type, myGame.current.rot, myGame.current.x, myGame.current.y + 1))
                    myGame.current.y++;
                gravityTimer = 0;
                break;
            case KEY_UP:
            case ' ':
                rotateCurrentPiece(&myGame);
                break;
            case 'q':
            case 'Q':
                myGame.game_over = 1; // Trigger game over sequence instead of immediate break
                break;
            }
        }

        // 2. Gravity
        gravityTimer++;
        if (gravityTimer >= gravityThreshold)
        {
            tickGame(&myGame);
            gravityTimer = 0;
        }

        // 3. Render
        drawBoard(&myGame);

        // 4. Frame Delay
        usleep(10000);
    }

    // --- POST-GAME CLEANUP ---
    drawBoard(&myGame); // Final draw
    mvprintw(BOARD_HEIGHT + 6, 0, "<!> ====================== <!>");
    mvprintw(BOARD_HEIGHT + 7, 0, "<!>       GAME OVER!       <!>");
    mvprintw(BOARD_HEIGHT + 8, 0, "<!>    Final Score: %-4d   <!>", myGame.score);
    mvprintw(BOARD_HEIGHT + 9, 0, "<!> ====================== <!>");
    mvprintw(BOARD_HEIGHT + 11, 0, "Press any key to exit...");
    refresh();

    // Wait for player to press a key before exiting
    nodelay(stdscr, FALSE); // Turn blocking back ON so we just pause here
    getch();

    endwin(); // CRITICAL: Restores the user's terminal back to normal
    return 0;
}