#ifndef FAN_STATE_H
#define FAN_STATE_H

#include "fan_common.h"

void fan_state_init(fan_state_t *state);
bool handle_event(fan_state_t *state, fan_event_t event);

#endif /* FAN_STATE_H */
