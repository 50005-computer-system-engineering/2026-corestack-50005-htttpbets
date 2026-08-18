#include <stdarg.h>
#include <stddef.h>
#include <cmocka.h>
#include <stdlib.h>
#include "../lib/libbombbrain/inventory.h"

#pragma region Test Group 1: consume_bomb
// Pure function, no setup/teardown needed

// [Correctness] Consuming a bomb should decrement remaining_bombs by 1
static void test_consume_bomb_decrements_remaining(void **state) {
    (void)state;
    InventoryStock stock = {.remaining_bombs = 3, .num_bombs = 3, .num_fires = 1, .bomb_replenish_timer = 0.0f};
    consume_bomb(&stock);
    assert_int_equal(stock.remaining_bombs, 2);
}

// [Correctness] Consuming a bomb should (re)start the replenish timer
static void test_consume_bomb_resets_replenish_timer(void **state) {
    (void)state;
    InventoryStock stock = {.remaining_bombs = 2, .num_bombs = 3, .num_fires = 1, .bomb_replenish_timer = 0.2f};
    consume_bomb(&stock);
    assert_float_equal(stock.bomb_replenish_timer, 1.0f, 0.001f);
}
#pragma endregion Test Group 1

#pragma region Test Group 2: inventory_update
// Pure function, no setup/teardown needed

// [Boundary] Stock already full should not tick the replenish timer down
static void test_inventory_update_noop_when_full(void **state) {
    (void)state;
    InventoryStock stock = {.remaining_bombs = 3, .num_bombs = 3, .num_fires = 1, .bomb_replenish_timer = 0.5f};
    inventory_update(&stock, 0.5f);
    assert_int_equal(stock.remaining_bombs, 3);
    assert_float_equal(stock.bomb_replenish_timer, 0.5f, 0.001f);
}

// [Scaling] Timer should tick down while under capacity, without replenishing yet
static void test_inventory_update_ticks_timer_down(void **state) {
    (void)state;
    InventoryStock stock = {.remaining_bombs = 1, .num_bombs = 3, .num_fires = 1, .bomb_replenish_timer = 1.0f};
    inventory_update(&stock, 0.4f);
    assert_int_equal(stock.remaining_bombs, 1);
    assert_float_equal(stock.bomb_replenish_timer, 0.6f, 0.001f);
}

// [Correctness] Once the timer expires, a bomb should be replenished and the timer reset
static void test_inventory_update_replenishes_on_expiry(void **state) {
    (void)state;
    InventoryStock stock = {.remaining_bombs = 1, .num_bombs = 3, .num_fires = 1, .bomb_replenish_timer = 0.1f};
    inventory_update(&stock, 0.2f);
    assert_int_equal(stock.remaining_bombs, 2);
    assert_float_equal(stock.bomb_replenish_timer, 1.0f, 0.001f);
}
#pragma endregion Test Group 2

int main(void) {
    const struct CMUnitTest consume_bomb_tests[] = {
        cmocka_unit_test(test_consume_bomb_decrements_remaining),
        cmocka_unit_test(test_consume_bomb_resets_replenish_timer),
    };

    const struct CMUnitTest inventory_update_tests[] = {
        cmocka_unit_test(test_inventory_update_noop_when_full),
        cmocka_unit_test(test_inventory_update_ticks_timer_down),
        cmocka_unit_test(test_inventory_update_replenishes_on_expiry),
    };

    int failed = 0;
    failed += cmocka_run_group_tests_name("consume_bomb", consume_bomb_tests, NULL, NULL);
    failed += cmocka_run_group_tests_name("inventory_update", inventory_update_tests, NULL, NULL);
    return failed;
}
