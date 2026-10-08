#include <errno.h>

#include "fan_output.h"

int fan_output_init(void)
{
    /* TODO (Member 3): open /dev/fan_pwm and keep the descriptor. */
    errno = ENOSYS;
    return -1;
}

int apply_fan_state(const fan_state_t *state)
{
    /* TODO (Member 3): validate state, convert to int speed, write payload.
     * TODO: handle interrupted/short writes and device errors.
     */
    (void)state;
    errno = ENOSYS;
    return -1;
}

void fan_output_cleanup(void)
{
    /* TODO (Member 3): stop output and close any acquired descriptor. */
}
