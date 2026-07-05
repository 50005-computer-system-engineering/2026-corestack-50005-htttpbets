#ifndef TETRISBRAIN_H
#define TETRISBRAIN_H

#include <stdint.h>
#include <stdbool.h>

/* ----- BOARD LOGISTICS ----- */
#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20

/* ----- TETRIS PIECES ----- */
typedef enum
{
    PIECE_NONE = 0,
    PIECE_I = 1, // Straight line of 4 blocks
    PIECE_O = 2, // Square block
    PIECE_T = 3, // T-piece
    PIECE_S = 4, // Z shape, offset to the right
    PIECE_Z = 5, // Z shape, offset to the left
    PIECE_J = 6, // Reverse L shape pointing to the left
    PIECE_L = 7  // Reverse L shape pointing to the right
} PieceType;

/* ----- ROTATION (SUPER ROTATION SYSTEM) ----- */
typedef enum
{
    ROT_0 = 0, // SPAWN
    ROT_1 = 1, // RIGHT
    ROT_2 = 2, // 180 DEG
    ROT_3 = 3  // LEFT
} Rotation;

/* ----- ACTIVE PIECE ----- */
typedef struct
{
    PieceType type;
    Rotation rot;
    int x; // Location on the board (x-axis)
    int y; // Location on the board (y-axis)
} Piece;

/* ----- BOARD ----- */
typedef struct
{
    uint8_t cells[BOARD_HEIGHT][BOARD_WIDTH];
} Board;

/* ----- GAME STATE ----- */
typedef struct
{
    Board board;
    Piece current;
    Piece next;
    int held_type;
    bool has_held;
    int bag[7];
    int bag_index;
    int pieces_placed;
    int tetrises;
    int t_spins;
    bool last_action_rotation;
    int score;
    int lines_cleared;
    int level;
    int game_over;
} GameState;

extern const int tetrominoes[7][16];

// function call to start the game
void startGame(GameState *state);

// Bagging system - fill the bag with the pieces and shuffle them randomly
void shuffleBag(GameState *state);

// Spawns a new piece
void spawnNewPiece(GameState *state);

// Convert 2D (x,y) coordinates into 1D index for array
int getRotationIndex(int x, int y, Rotation rot);

// Check for collisions
bool isValidPos(GameState *state, PieceType type, Rotation rot, int posX, int posY);

// Wall kick helper function
bool testRotate(GameState *state, int nextRot);

// Rotate clockwise logic
void rotateCurrentPiece(GameState *state);

// Rotate counter clockwise logic
void rotateCounterClockwise(GameState *state);

// Locking the piece after it finalizes its position
void lockPiece(GameState *state);

// Tetris
int clearLines(GameState *state);

// Check for t-spin; return 1 if valid, 0 if not
int checkTSpin(GameState *state);

// Repeat function to advance the game
int tickGame(GameState *state);

// Hold piece
void holdPiece(GameState *state);

#endif