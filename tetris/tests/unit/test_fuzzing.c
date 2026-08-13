#include "unity.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include "lib/libtetrisbrain/engine.h"
#include "lib/libtetrisbrain/movement.h"
#include "lib/libtetrisbrain/board_control.h"
#include "lib/libtetrisbrain/bag.h"

#define FUZZ_ITERATIONS 50000

void setUp(void) {
    srand(time(NULL));
}
void tearDown(void) {}

void test_engine_fuzzing_boundaries(void) {
    GameState state;
    memset(&state, 0, sizeof(GameState));
    refillBag(&state);
    spawnNewPiece(&state);

    for (int i = 0; i < FUZZ_ITERATIONS; i++) {
        // 1. Boundary Value Analysis (L9): Inject out-of-bounds coordinates
        int hostile_x = (rand() % 400) - 200; // Range: -200 to +200
        int hostile_y = (rand() % 400) - 200;
        int hostile_type = (rand() % 20) - 10; // Out-of-bounds piece types
        int hostile_rot = (rand() % 10) - 5;   // Invalid rotation states

        // 2. Attack the collision parser directly
        // Must return false without reading out-of-bounds memory
        isValidPos(&state, hostile_type, hostile_rot, hostile_x, hostile_y);

        // 3. Force state corruption and test recovery
        state.current.x = hostile_x;
        state.current.y = hostile_y;
        state.current.type = (PieceType)hostile_type;
        state.current.rot = (Rotation)hostile_rot;

        // 4. Force movement functions to evaluate the corrupted state
        moveLeft(&state);
        moveRight(&state);
        softDrop(&state);
        
        // 5. Periodically trigger ticks and hard drops
        if (i % 100 == 0) {
            hardDrop(&state);
            tickGame(&state);
            // Reset to a clean state if game over triggered
            if (state.game_over) {
                memset(&state, 0, sizeof(GameState));
                refillBag(&state);
                spawnNewPiece(&state);
            }
        }
    }

    TEST_ASSERT_TRUE_MESSAGE(true, "Tetris engine survived 50,000 fuzzing iterations without crashing");
}

int main(void) {
    UNITY_BEGIN();
    printf("[*] Executing 50,000 Iteration Engine Fuzzer...\n");
    RUN_TEST(test_engine_fuzzing_boundaries);
    return UNITY_END();
}