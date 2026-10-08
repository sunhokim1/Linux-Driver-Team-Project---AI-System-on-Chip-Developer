#include "fan_state.h"

static void set_mode(fan_state_t *state, fan_mode_t mode)
{
    state->mode = mode;
    state->power = mode != FAN_MODE_OFF;

    switch (mode) {
    case FAN_MODE_S1:
        state->speed = FAN_SPEED_S1;
        break;
    case FAN_MODE_S2:
        state->speed = FAN_SPEED_S2;
        break;
    case FAN_MODE_S3:
        state->speed = FAN_SPEED_S3;
        break;
    case FAN_MODE_OFF:
    default:
        state->speed = FAN_SPEED_MIN;
        break;
    }
}

void fan_state_init(fan_state_t *state)
{
    if (!state)
        return;

    set_mode(state, FAN_MODE_OFF);
}

bool handle_event(fan_state_t *state, fan_event_t event)
{
    fan_state_t previous;

    if (!state || event < EVENT_NONE || event > EVENT_LONG_PRESS)
        return false;

    previous = *state;

    if (state->mode < FAN_MODE_OFF || state->mode > FAN_MODE_S3) {
        /* Corrupt state fails closed, even if a short press is pending. */
        set_mode(state, FAN_MODE_OFF);
    } else {
        fan_mode_t next = state->mode;

        if (event == EVENT_LONG_PRESS)
            next = FAN_MODE_OFF;
        else if (event == EVENT_SHORT_PRESS) {
            switch (state->mode) {
            case FAN_MODE_OFF:
            case FAN_MODE_S3:
                next = FAN_MODE_S1;
                break;
            case FAN_MODE_S1:
                next = FAN_MODE_S2;
                break;
            case FAN_MODE_S2:
                next = FAN_MODE_S3;
                break;
            }
        }
        /* Recompute outputs even for EVENT_NONE to restore consistency. */
        set_mode(state, next);
    }

    return previous.mode != state->mode || previous.power != state->power ||
           previous.speed != state->speed;
}
