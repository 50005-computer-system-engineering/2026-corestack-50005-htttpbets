#include "unity.h"
#include <string.h>
#include "lib/libtetrisbrain/engine.h"
#include "lib/libtetrisbrain/movement.h"
#include "lib/libtetrisbrain/board_control.h"
#include "lib/libtetrisbrain/bag.h"

void setUp(void) {}
void tearDown(void) {}

void test_engine_movement_integration(void) {
    GameState state;
    memset(&state, 0, sizeof(GameState));
    
    // 1. Spawn a fresh piece
    refillBag(&state);
    spawnNewPiece(&state);
    
    int initial_x = state.current.x;
    
    // 2. Execute movement command
    bool moved = moveLeft(&state);
    
    // 3. Assert the state transitioned consistently
    TEST_ASSERT_TRUE_MESSAGE(moved, "moveLeft returned false on open board");
    TEST_ASSERT_EQUAL_INT_MESSAGE(initial_x - 1, state.current.x, "X position did not decrement correctly");
    TEST_ASSERT_FALSE_MESSAGE(state.last_action_rotation, "moveLeft failed to reset last_action_rotation to false");
}

void test_hardDrop_and_tick_integration(void) {
    GameState state;
    memset(&state, 0, sizeof(GameState));
    
    refillBag(&state);
    spawnNewPiece(&state);
    
    // 1. Hard drop the piece to the floor
    int rows_dropped = hardDrop(&state);
    TEST_ASSERT_TRUE_MESSAGE(rows_dropped >= 0, "hardDrop returned invalid row count");
    
    // 2. Advance the game tick to trigger locking and line clearing
    tickGame(&state);
    
    // 3. Assert the piece locked and incremented the placed counter
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, state.pieces_placed, "tickGame failed to increment pieces_placed after lock");
}

void test_tspin_detection(void) {
    GameState state;
    memset(&state, 0, sizeof(GameState));
    
    // Set current piece to T-Piece (Type 3)
    state.current.type = 3;
    state.current.x = 4;
    state.current.y = 10;
    state.last_action_rotation = true; // Required condition 1 & 2
    
    // Manually block 3 corners around the T-piece center
    // Center is (4+1, 10+1) = (5, 11)
    state.board.cells[10][4] = 1; // Top-Left
    state.board.cells[10][6] = 1; // Top-Right
    state.board.cells[12][4] = 1; // Bottom-Left
    
    // Assert checkTSpin evaluates to 1 (T-Spin confirmed)
    int is_tspin = checkTSpin(&state);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, is_tspin, "checkTSpin failed to identify valid 3-corner T-Spin");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_engine_movement_integration);
    RUN_TEST(test_hardDrop_and_tick_integration);
    RUN_TEST(test_tspin_detection);
    return UNITY_END();
}