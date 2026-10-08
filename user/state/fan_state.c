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

    state->near = false;
    state->blocked = false;
    set_mode(state, FAN_MODE_OFF);
}

bool handle_event(fan_state_t *state, fan_event_t event)
{
    fan_state_t previous;

    if (!state ||
        event < EVENT_NONE ||
        event > EVENT_SENSOR_LOST ||
        event == EVENT_ERROR)
        return false;

    previous = *state;

    if (state->mode < FAN_MODE_OFF ||
        state->mode > FAN_MODE_S3) {
        fan_state_init(state);
        state->blocked = true;
    } else {
        switch (event) {
        case EVENT_NEAR:
            state->near = true;

            if (!state->blocked &&
                state->mode == FAN_MODE_OFF)
                set_mode(state, FAN_MODE_S1);
            break;

        case EVENT_FAR:
            state->near = false;
            state->blocked = false;
            set_mode(state, FAN_MODE_OFF);
            break;

        case EVENT_SENSOR_LOST:
            state->near = false;
    /* 센서 오류 때문에 새로 차단하지 않음.
     * 버튼으로 정지한 blocked 상태는 유지함.
     */
         set_mode(state, FAN_MODE_OFF);
         break;

        case EVENT_LONG_PRESS:
            state->blocked = true;
            set_mode(state, FAN_MODE_OFF);
            break;

        case EVENT_SPEED_UP:
            if (state->near && !state->blocked) {
                if (state->mode == FAN_MODE_S1)
                    set_mode(state, FAN_MODE_S2);
                else if (state->mode == FAN_MODE_S2)
                    set_mode(state, FAN_MODE_S3);
            }
            break;

        case EVENT_SPEED_DOWN:
            if (state->near && !state->blocked) {
                if (state->mode == FAN_MODE_S3)
                    set_mode(state, FAN_MODE_S2);
                else if (state->mode == FAN_MODE_S2)
                    set_mode(state, FAN_MODE_S1);
            }
            break;

        case EVENT_NONE:
        case EVENT_SHORT_PRESS:
        default:
            break;
        }

        set_mode(state, state->mode);
    }

    /* 모터 출력이나 표시가 달라질 때만 true */
    return previous.mode != state->mode ||
           previous.power != state->power ||
           previous.speed != state->speed;
}