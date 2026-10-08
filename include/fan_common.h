#ifndef FAN_COMMON_H
#define FAN_COMMON_H

/* Shared device protocol constants; payloads are native int values. */
#define ULTRASONIC_DEVICE_PATH "/dev/ultrasonic"
#define ENCODER_DEVICE_PATH "/dev/fan_encoder"
#define FAN_PWM_DEVICE_PATH "/dev/fan_pwm"
#define FAN_LED_DEVICE_PATH "/dev/fan_led"
#define FAN_SPEED_MIN 0
#define FAN_SPEED_ON_MIN 1
#define FAN_SPEED_MAX 8
#define FAN_SPEED_S1 2
#define FAN_SPEED_S2 5
#define FAN_SPEED_S3 8
#define FAN_LONG_PRESS_MS 2000

/* Kernel code uses only the protocol constants above. */
#ifndef __KERNEL__
#include <stdbool.h>

typedef enum {
    EVENT_NONE = 0,
    /* Member 1: emit SHORT once on release before 2000 ms.
     * Emit LONG once at >= 2000 ms; suppress SHORT on that release.
     * Use a monotonic clock and debounce the input in the Event module.
     */
    EVENT_SHORT_PRESS,
    EVENT_LONG_PRESS,
    /* Input/I/O failure; EVENT_NONE means no new button event. */
    EVENT_ERROR,
    EVENT_SPEED_UP,
    EVENT_SPEED_DOWN,
    EVENT_NEAR,
    EVENT_FAR,
    EVENT_SENSOR_LOST,
    EVENT_POWER_TOGGLE,
    /* Compatibility names for encoder callers. */
    EVENT_ROTATE_CW = EVENT_SPEED_UP,
    EVENT_ROTATE_CCW = EVENT_SPEED_DOWN
} fan_event_t;

typedef enum {
    FAN_MODE_OFF = 0,
    FAN_MODE_S1,
    FAN_MODE_S2,
    FAN_MODE_S3
} fan_mode_t;

typedef struct {
    /* Derived output fields retained for Member 3's existing interface.
     * Only the State module updates these fields and mode together.
     */
    bool power;
    int speed;
    fan_mode_t mode;
} fan_state_t;
#endif

#endif /* FAN_COMMON_H */
