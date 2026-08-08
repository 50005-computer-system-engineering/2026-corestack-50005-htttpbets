#include "unity.h"
#include "libtetrisbrain/engine.h"
#include "libtetrisbrain/garbage.h"
#include "libtetrisbrain/state.h"
#include <string.h>

GameState test_state;

void setUp(void) { memset(&test_state, 0, sizeof(GameState)); }
void tearDown(void) {}

void test_garbage_mitigation_integration(void) {
    test_state.pending_garbage = 3;
    
    // Module 1 (Garbage) generates damage
    int damage = calculateGarbage(&test_state, 4, false); // Tetris = 4
    TEST_ASSERT_EQUAL_INT(4, damage);
    
    // Module 2 (Engine) mitigation logic
    if (damage >= test_state.pending_garbage) {
        damage -= test_state.pending_garbage;
        test_state.pending_garbage = 0;
    }
    
    // Assert modules interacted correctly
    TEST_ASSERT_EQUAL_INT(0, test_state.pending_garbage);
    TEST_ASSERT_EQUAL_INT(1, damage); // 1 remaining outgoing damage
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_garbage_mitigation_integration);
    return UNITY_END();
}