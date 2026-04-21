#include "unity.h"
#include "hal_mock.h"
#include "../presence/presence.h"

// ── Fixtures ──────────────────────────────────────────────────────────────────

static HAL hal;

static void drive_to_present(void) {
    hal_mock_set_rfid(true);
    hal_mock_set_weight(true);
    for (int i = 0; i < PRESENCE_DEBOUNCE_PRESENT_SAMPLES; i++) {
        presence_tick(&hal);
    }
}

static void drive_ipad_to_present(void) {
    hal_mock_set_ipad_sensor(true);
    for (int i = 0; i < PRESENCE_DEBOUNCE_PRESENT_SAMPLES; i++) {
        presence_tick(&hal);
    }
}

// ── Initial State ─────────────────────────────────────────────────────────────

void test_presence_initial_state_after_init(void) {
    // setUp() calls presence_init() and hal_mock_reset()
    // so this tests the guaranteed starting state
    TEST_ASSERT_FALSE(presence_is_remote_present());
    TEST_ASSERT_FALSE(presence_is_ipad_present());
}

// ── REQ-PRES-001 ─────────────────────────────────────────────────────────────

void test_presence_both_sensors_true_is_present(void) {
    
    drive_to_present();

    TEST_ASSERT_TRUE(presence_is_remote_present());
}

void test_presence_rfid_only_is_absent(void) {

    hal_mock_set_rfid(true);
    hal_mock_set_weight(false);

    for (int i = 0; i < PRESENCE_DEBOUNCE_PRESENT_SAMPLES; i++) {
        presence_tick(&hal);
    }

    TEST_ASSERT_FALSE(presence_is_remote_present());
}

void test_presence_weight_only_is_absent(void) { 
    
    hal_mock_set_rfid(false);
    hal_mock_set_weight(true);

    for (int i = 0; i < PRESENCE_DEBOUNCE_PRESENT_SAMPLES; i++) {
        presence_tick(&hal);
    }

    TEST_ASSERT_FALSE(presence_is_remote_present());
 }

 void test_presence_neither_sensor_is_absent(void) { 
    
    hal_mock_set_rfid(false);
    hal_mock_set_weight(false);

    for (int i = 0; i < PRESENCE_DEBOUNCE_PRESENT_SAMPLES; i++) {
        presence_tick(&hal);
    }

    TEST_ASSERT_FALSE(presence_is_remote_present());
 }

// ── REQ-PRES-002 ─────────────────────────────────────────────────────────────

void test_presence_remote_present_debounce_insufficient_samples(void) {
    // Setup required pre conditions
    hal_mock_set_rfid(true);
    hal_mock_set_weight(true);

    for (int i = 0; i < PRESENCE_DEBOUNCE_PRESENT_SAMPLES - 1; i++) {
        presence_tick(&hal);
    }    

    TEST_ASSERT_FALSE(presence_is_remote_present());
}

void test_presence_remote_present_debounce_sufficient_samples(void) {
    // Setup required pre conditions
    hal_mock_set_rfid(true);
    hal_mock_set_weight(true);

    for (int i = 0; i < PRESENCE_DEBOUNCE_PRESENT_SAMPLES; i++) {
        presence_tick(&hal);
    }    

    TEST_ASSERT_TRUE(presence_is_remote_present());
}

void test_presence_remote_absent_debounce_insufficient_samples(void) {
    // establish PRESENT via real logic
    drive_to_present();

    // N-1 absent samples — should still be PRESENT
    hal_mock_set_rfid(false);
    hal_mock_set_weight(false);

    for (int i = 0; i < PRESENCE_DEBOUNCE_ABSENT_SAMPLES - 1; i++) {
        presence_tick(&hal);
    }

    TEST_ASSERT_TRUE(presence_is_remote_present());
}

void test_presence_remote_absent_debounce_sufficient_samples(void) {
    // establish PRESENT via real logic
    drive_to_present();

    // exactly N absent samples — should now be ABSENT
    hal_mock_set_rfid(false);
    hal_mock_set_weight(false);

    for (int i = 0; i < PRESENCE_DEBOUNCE_ABSENT_SAMPLES; i++) {
        presence_tick(&hal);
    }

    TEST_ASSERT_FALSE(presence_is_remote_present());
}

void test_presence_remote_present_counter_resets_on_absent_sample_negative(void) {
    // Setup required pre conditions
    hal_mock_set_rfid(true);
    hal_mock_set_weight(true);

    for (int i = 0; i < PRESENCE_DEBOUNCE_PRESENT_SAMPLES - 1; i++) {
        presence_tick(&hal);
    }

    // read negative condition -> should reset counter
    hal_mock_set_rfid(false);
    hal_mock_set_weight(true);

    for (int i = 0; i < PRESENCE_DEBOUNCE_PRESENT_SAMPLES - 1; i++) {
        presence_tick(&hal);
    }

    // present state should remain absent
    TEST_ASSERT_FALSE(presence_is_remote_present());
}

void test_presence_remote_present_counter_resets_on_absent_sample_positive(void) {
    // Setup required pre conditions
    hal_mock_set_rfid(true);
    hal_mock_set_weight(true);

    for (int i = 0; i < PRESENCE_DEBOUNCE_PRESENT_SAMPLES - 1; i++) {
        presence_tick(&hal);
    }

    // read negative condition -> should reset counter
    hal_mock_set_rfid(false);
    hal_mock_set_weight(true);

    for (int i = 0; i < PRESENCE_DEBOUNCE_PRESENT_SAMPLES; i++) {
        presence_tick(&hal);
    }

    // present state should switch to absent
    TEST_ASSERT_FALSE(presence_is_remote_present());
}

void test_presence_remote_absent_counter_resets_on_present_sample_negative(void) {
    // establish PRESENT via real logic
    drive_to_present();

    // set absent condition
    hal_mock_set_rfid(false);
    hal_mock_set_weight(true);

    // read absent condition for an insufficent amount of samples
    for (int i = 0; i < PRESENCE_DEBOUNCE_ABSENT_SAMPLES - 1; i++) {
        presence_tick(&hal);
    }

    // present state should stay present
    TEST_ASSERT_TRUE(presence_is_remote_present());

    // now we read one present condition again  -> should reset counter
    hal_mock_set_rfid(true);
    hal_mock_set_weight(true);
    presence_tick(&hal);

    // if we now read the required amount of samples -1 again, we should 
    // still be reqding remote present.
    hal_mock_set_rfid(false);
    hal_mock_set_weight(true);

     for (int i = 0; i < PRESENCE_DEBOUNCE_ABSENT_SAMPLES - 1; i++) {
        presence_tick(&hal);
    }

    // present state should stay present
    TEST_ASSERT_TRUE(presence_is_remote_present());
    
}

void test_presence_remote_absent_counter_resets_on_present_sample_positive(void) {
    // establish PRESENT via real logic
    drive_to_present();

    // set absent condition
    hal_mock_set_rfid(false);
    hal_mock_set_weight(true);

    // read absent condition for an insufficent amount of samples
    for (int i = 0; i < PRESENCE_DEBOUNCE_ABSENT_SAMPLES - 1; i++) {
        presence_tick(&hal);
    }

    // present state should stay present
    TEST_ASSERT_TRUE(presence_is_remote_present());

    // now we read one present condition again  -> should reset counter
    hal_mock_set_rfid(true);
    hal_mock_set_weight(true);
    presence_tick(&hal);

    // if we now read the required amount of samples, we should 
    // switch states to absent.
    hal_mock_set_rfid(false);
    hal_mock_set_weight(true);
    
     for (int i = 0; i < PRESENCE_DEBOUNCE_ABSENT_SAMPLES; i++) {
        presence_tick(&hal);
    }

    // present state should stay present
    TEST_ASSERT_FALSE(presence_is_remote_present());
}

// ── REQ-PRES-003 ─────────────────────────────────────────────────────────────

void test_ipad_present_debounce_sufficient_samples(void) {
    
    drive_ipad_to_present();

    TEST_ASSERT_TRUE(presence_is_ipad_present());
}

void test_ipad_absent_debounce_insufficient_samples(void) {
    drive_ipad_to_present();

    hal_mock_set_ipad_sensor(false);
    for (int i = 0; i < PRESENCE_DEBOUNCE_ABSENT_SAMPLES - 1; i++) {
        presence_tick(&hal);
    }

    TEST_ASSERT_TRUE(presence_is_ipad_present());
}

void test_ipad_absent_debounce_sufficient_samples(void) {
    drive_ipad_to_present();

    hal_mock_set_ipad_sensor(false);
    for (int i = 0; i < PRESENCE_DEBOUNCE_ABSENT_SAMPLES; i++) {
        presence_tick(&hal);
    }

    TEST_ASSERT_FALSE(presence_is_ipad_present());
}

void test_ipad_channel_independent_from_remote_remote_present_ipad_absent(void) {
    hal_mock_set_rfid(true);
    hal_mock_set_weight(true);
    hal_mock_set_ipad_sensor(false);

    for (int i = 0; i < PRESENCE_DEBOUNCE_PRESENT_SAMPLES; i++) {
        presence_tick(&hal);
    }

    TEST_ASSERT_FALSE(presence_is_ipad_present());
    TEST_ASSERT_TRUE(presence_is_remote_present());
}

void test_ipad_channel_independent_from_remote_remote_absent_ipad_present(void) {
    hal_mock_set_rfid(true);
    hal_mock_set_weight(false);
    hal_mock_set_ipad_sensor(true);

    for (int i = 0; i < PRESENCE_DEBOUNCE_PRESENT_SAMPLES; i++) {
        presence_tick(&hal);
    }

    TEST_ASSERT_TRUE(presence_is_ipad_present());
    TEST_ASSERT_FALSE(presence_is_remote_present());
}

// ── Runner ────────────────────────────────────────────────────────────────────

void setUp(void) { 
    hal = hal_mock_create();
    hal_mock_reset();
    presence_init();
 }

void tearDown(void) {}

int main(void) {
    UNITY_BEGIN();

    // ── Initial state ────────────────
    RUN_TEST(test_presence_initial_state_after_init);


    // ── REQ-PRES-001 ────────────────
    RUN_TEST(test_presence_both_sensors_true_is_present);
    RUN_TEST(test_presence_rfid_only_is_absent);
    RUN_TEST(test_presence_weight_only_is_absent);
    RUN_TEST(test_presence_neither_sensor_is_absent);

    // ── REQ-PRES-002 ────────────────
    RUN_TEST(test_presence_remote_present_debounce_insufficient_samples);
    RUN_TEST(test_presence_remote_present_debounce_sufficient_samples);
    RUN_TEST(test_presence_remote_absent_debounce_insufficient_samples);
    RUN_TEST(test_presence_remote_absent_debounce_sufficient_samples);
    RUN_TEST(test_presence_remote_absent_counter_resets_on_present_sample_negative);
    RUN_TEST(test_presence_remote_absent_counter_resets_on_present_sample_positive);
    RUN_TEST(test_presence_remote_present_counter_resets_on_absent_sample_negative);
    RUN_TEST(test_presence_remote_present_counter_resets_on_absent_sample_positive);

    // ── REQ-PRES-003 ────────────────
    RUN_TEST(test_ipad_present_debounce_sufficient_samples);
    RUN_TEST(test_ipad_absent_debounce_insufficient_samples);
    RUN_TEST(test_ipad_absent_debounce_sufficient_samples);
    RUN_TEST(test_ipad_channel_independent_from_remote_remote_present_ipad_absent);
    RUN_TEST(test_ipad_channel_independent_from_remote_remote_absent_ipad_present);

    UNITY_END();
    return 0;
}