#include "unity.h"
#include "hal_mock.h"
#include "../lock/lock.h"
#include "../schedule/schedule.h"

// ── Fixtures ──────────────────────────────────────────────────────────────────

static HAL hal;

// ── Initial State ─────────────────────────────────────────────────────────────

void test_presence_initial_state_after_init(void) {
    TEST_ASSERT_TRUE(false);
}

// ── REQ-LOCK-001 ─────────────────────────────────────────────────────────────

void test_lock_idle_to_armed_remote_present(void) {

    TEST_ASSERT_TRUE(false);
}

// ── Runner ────────────────────────────────────────────────────────────────────

void setUp(void) { 
    hal = hal_mock_create();
    hal_mock_reset();
    //lock_init();
 }

void tearDown(void) {}

int main(void) {
    UNITY_BEGIN();

    // ── Initial state ────────────────
    RUN_TEST(test_presence_initial_state_after_init);
    
    // ── REQ-LOCK-001 ────────────────

    UNITY_END();
    return 0;
}