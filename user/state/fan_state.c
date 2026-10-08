#include "fan_state.h"

void fan_state_init(fan_state_t *state)
{
    state->power = false;
    state->speed = 0;
}

bool handle_event(fan_state_t *state, fan_event_t event)
{
    /* TODO (Member 2): implement the transition table.
     * TODO: enforce OFF => speed 0, ON => speed 1..8.
     * TODO: return true only when the state changes.
     */
    (void)state;
    (void)event;
    return false;
}
