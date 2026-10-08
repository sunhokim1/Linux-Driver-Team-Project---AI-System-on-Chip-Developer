#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>

#include "state/fan_state.h"

static const fan_state_t modes[] = {
    {.mode = FAN_MODE_OFF, .power = false, .speed = 0},
    {.mode = FAN_MODE_S1, .power = true, .speed = 2},
    {.mode = FAN_MODE_S2, .power = true, .speed = 5},
    {.mode = FAN_MODE_S3, .power = true, .speed = 8}
};

static void expect_transition(fan_state_t before, fan_event_t event,
                              fan_mode_t expected, bool changed)
{
    assert(handle_event(&before, event) == changed);
    assert(before.mode == expected);
    assert(before.power == modes[expected].power);
    assert(before.speed == modes[expected].speed);
}

static void test_transition_table(void)
{
    const fan_mode_t short_targets[] = {
        FAN_MODE_S1, FAN_MODE_S2, FAN_MODE_S3, FAN_MODE_S1
    };

    for (size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); ++i) {
        expect_transition(modes[i], EVENT_NONE, modes[i].mode, false);
        expect_transition(modes[i], EVENT_SHORT_PRESS, short_targets[i], true);
        expect_transition(modes[i], EVENT_LONG_PRESS, FAN_MODE_OFF,
                          modes[i].mode != FAN_MODE_OFF);
        expect_transition(modes[i], EVENT_ERROR, modes[i].mode, false);
        expect_transition(modes[i], (fan_event_t)-1, modes[i].mode, false);
        expect_transition(modes[i], (fan_event_t)100, modes[i].mode, false);
    }
}

static void test_invalid_state(void)
{
    const int corrupt_speeds[] = {INT_MIN, -1, 1, 9, INT_MAX};

    fan_state_init(NULL);
    assert(!handle_event(NULL, EVENT_SHORT_PRESS));
    for (size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); ++i) {
        for (size_t j = 0; j < sizeof(corrupt_speeds) / sizeof(corrupt_speeds[0]); ++j) {
            fan_state_t corrupt = modes[i];

            corrupt.power = !corrupt.power;
            corrupt.speed = corrupt_speeds[j];
            expect_transition(corrupt, EVENT_NONE, modes[i].mode, true);
        }
    }
    const fan_state_t invalid[] = {
        {.mode = (fan_mode_t)-1, .power = true, .speed = 8},
        {.mode = (fan_mode_t)100, .power = true, .speed = 5}
    };
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        expect_transition(invalid[i], EVENT_NONE, FAN_MODE_OFF, true);
        expect_transition(invalid[i], EVENT_SHORT_PRESS, FAN_MODE_OFF, true);
        expect_transition(invalid[i], EVENT_LONG_PRESS, FAN_MODE_OFF, true);
    }
}

static void test_sequence(void)
{
    fan_state_t state = modes[FAN_MODE_S3];

    fan_state_init(&state);
    assert(state.mode == FAN_MODE_OFF && !state.power && state.speed == 0);
    assert(handle_event(&state, EVENT_SHORT_PRESS));
    assert(state.mode == FAN_MODE_S1 && state.speed == 2);

    /* Repeat enough cycles to verify S3 wraps to S1 instead of OFF. */
    for (int cycle = 0; cycle < 10; ++cycle) {
        assert(handle_event(&state, EVENT_SHORT_PRESS));
        assert(state.mode == FAN_MODE_S2 && state.speed == 5);
        assert(handle_event(&state, EVENT_SHORT_PRESS));
        assert(state.mode == FAN_MODE_S3 && state.speed == 8);
        assert(handle_event(&state, EVENT_SHORT_PRESS));
        assert(state.mode == FAN_MODE_S1 && state.speed == 2);
    }
    assert(handle_event(&state, EVENT_LONG_PRESS));
    assert(state.mode == FAN_MODE_OFF && !state.power && state.speed == 0);
    assert(!handle_event(&state, EVENT_LONG_PRESS));
    assert(!handle_event(&state, EVENT_NONE));
    assert(handle_event(&state, EVENT_SHORT_PRESS));
    assert(state.mode == FAN_MODE_S1 && state.speed == 2);
}

int main(void)
{
    test_transition_table();
    test_invalid_state();
    test_sequence();
    puts("PASS: OFF/S1/S2/S3 transitions, output mapping and invalid states");
    return 0;
}
