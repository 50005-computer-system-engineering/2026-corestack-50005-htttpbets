#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include "libeventbus.h"
#include "tetrisu/config.h"
#include "tetrisu/events.h"
#include "tetrisu/input.h"
#include "tetrisu/renderer.h"
#include "tetrisbrain/state.h"

// --- MAIN GAME LOOP ---
int main()
{
    // Clear terminal screen
    // Set up the terminal for the game
    enableRawMode();
    printf("\e[1;1H\e[2J");
    fflush(stdout);

    startGame(&gamestate_p1);
    gamestate_p1.held_type = 0; // Initialize hold box
    gamestate_p1.has_held = false;

    startGame(&gamestate_p2);
    gamestate_p2.held_type = 0; // Initialize hold box
    gamestate_p2.has_held = false;

    // Event Bus setup
    event_bus_init(EVENT_COUNT);
    event_bus_listen(EVENT_ATTACK_GENERATED, on_attack_generated);

    // Timings set up => make the game more playable
    int p1_gravityTimer = 0;
    int p1_lockTimer = 0;
    int p2_gravityTimer = 0;
    int p2_lockTimer = 0;

    // --- THE GAME LOOP ---
    while (!gamestate_p1.game_over && !gamestate_p2.game_over)
    {
        // Track current gravity of piece for lock delay
        int p1_current_gravity = GRAVITY_THRESHOLD_START - ((gamestate_p1.level - 1) * 5);
        if (p1_current_gravity < 5)
        {
            p1_current_gravity = 5;
        }

        int p2_current_gravity = GRAVITY_THRESHOLD_START - ((gamestate_p2.level - 1) * 5);
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
                            rotateCurrentPiece(&gamestate_p2);
                            p2_lockTimer = 0;
                            break;
                        case 'D': // Left arrow
                            if (isValidPos(&gamestate_p2, gamestate_p2.current.type, gamestate_p2.current.rot, gamestate_p2.current.x - 1, gamestate_p2.current.y))
                            {
                                gamestate_p2.current.x--;
                                gamestate_p2.last_action_rotation = false;
                                p2_lockTimer = 0;
                            }
                            break;
                        case 'C': // Right arrow
                            if (isValidPos(&gamestate_p2, gamestate_p2.current.type, gamestate_p2.current.rot, gamestate_p2.current.x + 1, gamestate_p2.current.y))
                            {
                                gamestate_p2.current.x++;
                                gamestate_p2.last_action_rotation = false;
                                p2_lockTimer = 0;
                            }
                            break;
                        case 'B': // Down arrow (soft drop + lock delay)
                            if (isValidPos(&gamestate_p2, gamestate_p2.current.type, gamestate_p2.current.rot, gamestate_p2.current.x, gamestate_p2.current.y + 1))
                            {
                                gamestate_p2.current.y++;
                                gamestate_p2.last_action_rotation = false;
                                p2_gravityTimer = 0; // Reset timer to prevent double dropping
                            }
                            break;
                        }
                    }
                }
            }
            else if (key == 'x' || key == 'X') // Rotate clockwise (alternate key)
            {
                rotateCurrentPiece(&gamestate_p2);
                p2_lockTimer = 0;
            }
            else if (key == 'z' || key == 'Z') // Rotate counterclockwise
            {
                rotateCounterClockwise(&gamestate_p2);
                p2_lockTimer = 0;
            }
            else if (key == ' ') // Spacebar (hard drop)
            {
                while (isValidPos(&gamestate_p2, gamestate_p2.current.type, gamestate_p2.current.rot, gamestate_p2.current.x, gamestate_p2.current.y + 1))
                {
                    gamestate_p2.current.y++;
                    gamestate_p2.last_action_rotation = false;
                }

                // Calculate Garbage and send
                tickGame(&gamestate_p2);
                if (gamestate_p2.outgoing_garbage > 0)
                {
                    // Pack the payload
                    AttackPayload payload =
                        {
                            .source_player = 2,
                            .target_player = 1,
                            .lines = gamestate_p2.outgoing_garbage};
                    // Trigger Event Bus
                    event_bus_trigger(EVENT_ATTACK_GENERATED, &payload);
                    gamestate_p2.outgoing_garbage = 0; // Reset after sending
                }
                p2_gravityTimer = 0;
            }
            else if (key == 'h' || key == 'H') // H to hold
            {
                if (!gamestate_p2.has_held)
                {
                    holdPiece(&gamestate_p2);
                    p2_gravityTimer = 0;
                }
            }

            // --- PLAYER 1 (WASD) Controls ---
            else if (key == 'w' || key == 'W') // P1 Rotate CW
            {
                rotateCurrentPiece(&gamestate_p1);
                p1_lockTimer = 0;
            }
            else if (key == 'a' || key == 'A') // P1 Left
            {
                if (isValidPos(&gamestate_p1, gamestate_p1.current.type, gamestate_p1.current.rot, gamestate_p1.current.x - 1, gamestate_p1.current.y))
                {
                    gamestate_p1.current.x--;
                    gamestate_p1.last_action_rotation = false;
                    p1_lockTimer = 0;
                }
            }
            else if (key == 'd' || key == 'D') // P1 Right
            {
                if (isValidPos(&gamestate_p1, gamestate_p1.current.type, gamestate_p1.current.rot, gamestate_p1.current.x + 1, gamestate_p1.current.y))
                {
                    gamestate_p1.current.x++;
                    gamestate_p1.last_action_rotation = false;
                    p1_lockTimer = 0;
                }
            }
            else if (key == 's' || key == 'S') // P1 Down (Soft Drop)
            {
                if (isValidPos(&gamestate_p1, gamestate_p1.current.type, gamestate_p1.current.rot, gamestate_p1.current.x, gamestate_p1.current.y + 1))
                {
                    gamestate_p1.current.y++;
                    gamestate_p1.last_action_rotation = false;
                    p1_gravityTimer = 0;
                }
            }
            else if (key == 'g' || key == 'G') // P1 Hard Drop
            {
                while (isValidPos(&gamestate_p1, gamestate_p1.current.type, gamestate_p1.current.rot, gamestate_p1.current.x, gamestate_p1.current.y + 1))
                {
                    gamestate_p1.current.y++;
                    gamestate_p1.last_action_rotation = false;
                }

                // Calculate Garbage and send
                tickGame(&gamestate_p1);
                if (gamestate_p1.outgoing_garbage > 0)
                {
                    // Pack the payload
                    AttackPayload payload =
                        {
                            .source_player = 1,
                            .target_player = 2,
                            .lines = gamestate_p1.outgoing_garbage};
                    // Trigger Event Bus
                    event_bus_trigger(EVENT_ATTACK_GENERATED, &payload);
                    gamestate_p1.outgoing_garbage = 0; // Reset after sending
                }

                p1_gravityTimer = 0;
            }
            else if (key == 'f' || key == 'F') // P1 Hold
            {
                if (!gamestate_p1.has_held)
                {
                    holdPiece(&gamestate_p1);
                    p1_gravityTimer = 0;
                }
            }
            else if (key == 'q' || key == 'Q') // Q to quit
            {
                // Set game over states for both P1 and P2
                gamestate_p1.game_over = true;
                gamestate_p2.game_over = true;
                break; // Exit
            }
        }

        // Gravity + Lock Delay
        bool p1_resting = !isValidPos(&gamestate_p1, gamestate_p1.current.type, gamestate_p1.current.rot, gamestate_p1.current.x, gamestate_p1.current.y + 1);
        if (p1_resting)
        {
            // Lock Timer
            p1_lockTimer++;
            if (p1_lockTimer >= LOCK_THRESHOLD_START)
            {
                // Calculate Garbage and send
                tickGame(&gamestate_p1);
                if (gamestate_p1.outgoing_garbage > 0)
                {
                    // Pack the payload
                    AttackPayload payload =
                        {
                            .source_player = 1,
                            .target_player = 2,
                            .lines = gamestate_p1.outgoing_garbage};
                    // Trigger Event Bus
                    event_bus_trigger(EVENT_ATTACK_GENERATED, &payload);
                    gamestate_p1.outgoing_garbage = 0; // Reset after sending
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
                tickGame(&gamestate_p1);
                p1_gravityTimer = 0;
            }
        }

        // For player 2
        bool p2_resting = !isValidPos(&gamestate_p2, gamestate_p2.current.type, gamestate_p2.current.rot, gamestate_p2.current.x, gamestate_p2.current.y + 1);
        if (p2_resting)
        {
            // Lock Timer
            p2_lockTimer++;
            if (p2_lockTimer >= LOCK_THRESHOLD_START)
            {
                // Calculate Garbage and send
                tickGame(&gamestate_p2);
                if (gamestate_p2.outgoing_garbage > 0)
                {
                    // Pack the payload
                    AttackPayload payload =
                        {
                            .source_player = 2,
                            .target_player = 1,
                            .lines = gamestate_p2.outgoing_garbage};
                    // Trigger Event Bus
                    event_bus_trigger(EVENT_ATTACK_GENERATED, &payload);
                    gamestate_p2.outgoing_garbage = 0; // Reset after sending
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
                tickGame(&gamestate_p2);
                p2_gravityTimer = 0;
            }
        }

        // Render the boards
        drawBothBoards(&gamestate_p1, &gamestate_p2);

        // Delay frames to be visible to the human eye
        usleep(DELAY_MICROSECONDS);
    }

    // Clean up after game ends
    drawBothBoards(&gamestate_p1, &gamestate_p2);
    printf("\n\n");
    printf("<!> ====================== <!>\n");

    if (gamestate_p1.game_over && !gamestate_p2.game_over)
    {
        printf("<!>    PLAYER 2 WINS!      <!>\n");
    }
    else if (gamestate_p2.game_over && !gamestate_p1.game_over)
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

    while (!kbhit())
    {
        usleep(DELAY_MICROSECONDS);
    }
    getch();

    return 0;
}

/* uncomment this for single player

// Spawn a game state
GameState myGame;


// --- MAIN GAME LOOP ---
int main()
{
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

*/