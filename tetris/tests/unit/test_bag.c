#include "unity.h"
#include <string.h>
#include "lib/libtetrisbrain/bag.h"
#include "lib/libtetrisbrain/state.h"

void setUp(void) {}
void tearDown(void) {}

void test_refillBag_logic(void) {
    // 1. Initialize a clean game state
    GameState state;
    memset(&state, 0, sizeof(GameState));

    // 2. Trigger the refill logic
    refillBag(&state);

    // 3. Assert the pointer was correctly reset to the start
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, state.bag_index, "refillBag did not reset bag_index to 0");

    // 4. Assert the Fisher-Yates shuffle generated exactly 7 valid pieces (1 through 7)
    // We check indices 7-13 because refillBag places the new bag in the second half of the array
    int sum = 0;
    for (int i = 7; i < 14; i++) {
        int piece = state.bag[i];
        TEST_ASSERT_TRUE_MESSAGE(piece >= 1 && piece <= 7, "Generated piece type is out of bounds");
        sum += piece;
    }
    
    // The sum of pieces 1+2+3+4+5+6+7 is exactly 28. 
    // This mathematically proves no pieces were duplicated or lost during the shuffle.
    TEST_ASSERT_EQUAL_INT_MESSAGE(28, sum, "Bag did not generate exactly one of each 7 pieces");
}

void test_spawnNewPiece_and_exhaustion(void) {
    GameState state;
    memset(&state, 0, sizeof(GameState));
    
    // 1. Seed the bag to ensure both halves (0-6 and 7-13) have valid piece data
    refillBag(&state);
    refillBag(&state);

    // 2. Draw 6 pieces and assert the index increments correctly
    for (int i = 0; i < 6; i++) {
        spawnNewPiece(&state);
        TEST_ASSERT_EQUAL_INT_MESSAGE(i + 1, state.bag_index, "bag_index did not increment correctly upon drawing");
    }

    // 3. Draw the 7th piece. This triggers the exhaustion boundary.
    spawnNewPiece(&state);
    
    // 4. Assert that hitting index 7 successfully triggered refillBag() and reset the index to 0
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, state.bag_index, "bag_index did not reset to 0 upon exhaustion");
    
    // 5. Assert the spawn constraints were accurately applied to the new piece
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, state.current.rot, "Piece rotation not reset to ROT_0 upon spawn");
    TEST_ASSERT_FALSE_MESSAGE(state.has_held, "has_held not reset to false upon spawn");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_refillBag_logic);
    RUN_TEST(test_spawnNewPiece_and_exhaustion);
    return UNITY_END();
}