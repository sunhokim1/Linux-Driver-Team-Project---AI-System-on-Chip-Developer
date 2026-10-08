#include <stdio.h>

#include "fan_common.h"
#include "event/event_handler.h"
#include "state/fan_state.h"
#include "output/fan_output.h"

int main(void)
{
    fan_state_t state;

    fan_state_init(&state);

    if (fan_output_init() < 0) {
        perror("fan_output_init (skeleton)");
        fan_output_cleanup();
        return 1;
    }

    /* TODO (Member 2): initialize sensor and apply initial OFF output.
     * TODO: install termination signal handling.
     * TODO: loop over get_event() -> handle_event() -> apply_fan_state().
     * TODO: control measurement interval and stop safely on errors.
     * The skeleton does not start an unfinished infinite loop.
     */

    fan_output_cleanup();
    return 0;
}
