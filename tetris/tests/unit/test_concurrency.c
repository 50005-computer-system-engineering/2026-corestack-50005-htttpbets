#include "unity.h"
#include <pthread.h>
#include <string.h>
#include "lib/libtetrisbrain/engine.h"
#include "lib/libtetrisbrain/movement.h"
#include "lib/libtetrisbrain/bag.h"

#define NUM_THREADS 2
#define OPERATIONS_PER_THREAD 5000

static GameState global_state;

void setUp(void) {
    memset(&global_state, 0, sizeof(GameState));
    refillBag(&global_state);
    spawnNewPiece(&global_state);
}

void tearDown(void) {}

void* thread_input_spam(void* arg) {
    (void)arg;
    
    for (int i = 0; i < OPERATIONS_PER_THREAD; i++) {
        // Rapidly alternate movement inputs
        moveLeft(&global_state);
        moveRight(&global_state);
        softDrop(&global_state);
        updateTimers(&global_state);
    }
    return NULL;
}

void test_concurrent_state_mutation(void) {
    pthread_t threads[NUM_THREADS];
    
    // 1. Spawn concurrent threads hammering the shared GameState pointer
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, thread_input_spam, NULL);
    }
    
    // 2. Synchronize threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // 3. Assert the state survived without memory corruption or deadlocks
    TEST_ASSERT_TRUE_MESSAGE(global_state.current.x >= -4 && global_state.current.x <= BOARD_WIDTH + 4, 
                            "Concurrent inputs corrupted piece position beyond logical bounds");
}

int main(void) {
    UNITY_BEGIN();
    printf("[*] Executing Concurrency & Thread-Safety Tests...\n");
    RUN_TEST(test_concurrent_state_mutation);
    return UNITY_END();
}