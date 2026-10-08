#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "fan_output.h"

static int motor_fd = -1;

static int send_speed(int speed)
{
    ssize_t n;

    do {
        n = write(motor_fd, &speed, sizeof(speed));
    } while (n < 0 && errno == EINTR);

    if (n != sizeof(speed)) {
        if (n >= 0)
            errno = EIO;

        return -1;
    }

    return 0;
}

int fan_output_init(void)
{
    if (motor_fd >= 0) {
        errno = EBUSY;
        return -1;
    }

    motor_fd = open(
        FAN_PWM_DEVICE_PATH,
        O_WRONLY | O_CLOEXEC
    );

    if (motor_fd < 0)
        return -1;

    if (send_speed(0) < 0) {
        int saved = errno;

        close(motor_fd);
        motor_fd = -1;
        errno = saved;

        return -1;
    }

    return 0;
}

int apply_fan_state(const fan_state_t *state)
{
    int expected_speed;

    if (!state || motor_fd < 0) {
        errno = EINVAL;
        return -1;
    }

    switch (state->mode) {
    case FAN_MODE_OFF:
        expected_speed = FAN_SPEED_MIN;
        break;

    case FAN_MODE_S1:
        expected_speed = FAN_SPEED_S1;
        break;

    case FAN_MODE_S2:
        expected_speed = FAN_SPEED_S2;
        break;

    case FAN_MODE_S3:
        expected_speed = FAN_SPEED_S3;
        break;

    default:
        errno = EINVAL;
        return -1;
    }

    if (state->speed != expected_speed ||
        state->power != (expected_speed != 0)) {
        errno = EINVAL;
        return -1;
    }

    /* Member2 상태 값 0, 2, 5, 8을 그대로 전달 */
    return send_speed(state->speed);
}

void fan_output_cleanup(void)
{
    if (motor_fd >= 0) {
        /* 드라이버 release()에서도 모터 정지 */
        close(motor_fd);
        motor_fd = -1;
    }
}