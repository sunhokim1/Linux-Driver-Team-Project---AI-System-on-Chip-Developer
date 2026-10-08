#ifndef FAN_COMMON_H
#define FAN_COMMON_H

/* Shared device protocol constants; payloads are native int values. */
#define ULTRASONIC_DEVICE_PATH "/dev/ultrasonic"
#define FAN_PWM_DEVICE_PATH "/dev/fan_pwm"
#define FAN_SPEED_MIN 0
#define FAN_SPEED_MAX 8

/* Kernel code uses only the protocol constants above. */
#ifndef __KERNEL__
#include <stdbool.h>

typedef enum {
    EVENT_NONE = 0,
    EVENT_SPEED_UP,
    EVENT_SPEED_DOWN,
    EVENT_POWER_TOGGLE
} fan_event_t;

typedef struct {
    bool power;
    int speed;
} fan_state_t;
#endif

#endif /* FAN_COMMON_H */
