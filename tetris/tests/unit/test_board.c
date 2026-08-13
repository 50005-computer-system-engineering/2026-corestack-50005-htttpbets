#include "unity.h"
#include <string.h>
#include "lib/libtetrisbrain/board.h"
#include "lib/libtetrisbrain/board_control.h"
#include "lib/libtetrisbrain/state.h"

void setUp(void) {}
void tearDown(void) {}

void test_isValidPos_boundaries(void) {
    // 1. Initialize a clean game state
    GameState state;
    memset(&state, 0, sizeof(GameState));

    // 2. Test Extreme Boundaries (L9 Boundary Value Analysis)
    // We use type 1 (I-piece) and rotation 0 (ROT_0).
    // isValidPos expects: (GameState*, PieceType, Rotation, x, y)
    
    // Out of bounds left (-4 pushes the 4x4 matrix completely off the board)
    bool result_left = isValidPos(&state, 1, 0, -4, 5); 
    
    // Out of bounds right (BOARD_WIDTH is 10)[cite: 11, 13]
    bool result_right = isValidPos(&state, 1, 0, BOARD_WIDTH, 5); 
    
    // Out of bounds bottom (BOARD_HEIGHT is 20)[cite: 11, 13]
    bool result_bottom = isValidPos(&state, 1, 0, 5, BOARD_HEIGHT);

    // 3. Assert the engine actively rejected them
    TEST_ASSERT_FALSE_MESSAGE(result_left, "isValidPos accepted piece outside left boundary");
    TEST_ASSERT_FALSE_MESSAGE(result_right, "isValidPos accepted piece outside right boundary");
    TEST_ASSERT_FALSE_MESSAGE(result_bottom, "isValidPos accepted piece outside bottom boundary");
}

void test_clearLines_logic(void) {
    GameState state;
    memset(&state, 0, sizeof(GameState));

    // 1. Manually inject a fully solid line at the very bottom (Y = 19)[cite: 11]
    for (int x = 0; x < BOARD_WIDTH; x++) {
        // Any non-zero value represents a locked piece block
        state.board.cells[BOARD_HEIGHT - 1][x] = 1; 
    }
    
    // 2. Trigger the line clear check
    int lines_cleared = clearLines(&state); 
    
    // 3. Assert the logic caught exactly 1 line and removed it
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, lines_cleared, "Failed to clear exactly 1 line");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, state.board.cells[BOARD_HEIGHT - 1][0], "Cleared row was not emptied to 0");
    
    // 4. Assert the state score and level incremented properly
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, state.lines_cleared, "State lines_cleared counter not updated");
    TEST_ASSERT_EQUAL_INT_MESSAGE(100, state.score, "State score not updated by 100");
}

void test_lockPiece_logic(void) {
    GameState state;
    memset(&state, 0, sizeof(GameState));
    
    // 1. Manually configure the active piece in the state[cite: 13]
    state.current.type = 2;  // O-piece (Index 1 in tetrominoes array)[cite: 13]
    state.current.rot = 0;   // ROT_0
    state.current.x = 4;
    state.current.y = 18;    // Bottom of the board
    
    // 2. Lock the piece into the board array
    lockPiece(&state);
    
    // 3. Assert the O-piece blocks were written to the correct cells[cite: 13]
    // The O-piece at ROT_0 has solid blocks at local indices (1,1), (2,1), (1,2), (2,2)[cite: 13]
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, state.board.cells[19][5], "lockPiece failed to write piece type to board cells");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_isValidPos_boundaries);
    RUN_TEST(test_clearLines_logic);
    RUN_TEST(test_lockPiece_logic);
    return UNITY_END();
}