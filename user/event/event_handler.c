#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "event_handler.h"

struct encoder_sample {
    int steps;
    unsigned int pressed;
};

_Static_assert(sizeof(struct encoder_sample) == 8,
               "encoder ABI size");

static int sensor_fd = -1;
static int encoder_fd = -1;
static int initialized;

static int near_sensor;
static int key_held;
static int long_sent;
static struct timespec press_started;

static void cleanup(void)
{
    if (sensor_fd >= 0)
        close(sensor_fd);

    if (encoder_fd >= 0)
        close(encoder_fd);

    sensor_fd = -1;
    encoder_fd = -1;
}

static int initialize(void)
{
    int saved;

    encoder_fd = open(ENCODER_DEVICE_PATH,
                      O_RDONLY | O_CLOEXEC);

    if (encoder_fd < 0)
        return -1;

    sensor_fd = open(ULTRASONIC_DEVICE_PATH,
                     O_RDONLY | O_CLOEXEC);

    if (sensor_fd < 0) {
        saved = errno;
        cleanup();
        errno = saved;
        return -1;
    }

    if (atexit(cleanup) != 0) {
        cleanup();
        errno = ENOMEM;
        return -1;
    }

    initialized = 1;
    return 0;
}

static long elapsed_ms(const struct timespec *now)
{
    return (now->tv_sec - press_started.tv_sec) * 1000L
         + (now->tv_nsec - press_started.tv_nsec) / 1000000L;
}

fan_event_t get_event(void)
{
    struct encoder_sample sample;
    struct timespec now;
    int distance_mm;
    ssize_t n;

    if (!initialized && initialize() < 0)
        return EVENT_ERROR;

    /* 엔코더 회전량과 버튼 상태 읽기 */
    n = read(encoder_fd, &sample, sizeof(sample));

    if (n != (ssize_t)sizeof(sample)) {
        if (n >= 0)
            errno = EIO;

        return EVENT_ERROR;
    }

    printf("엔코더: steps=%d / pressed=%u\n",
           sample.steps, sample.pressed);
    fflush(stdout);

    if (clock_gettime(CLOCK_MONOTONIC, &now) < 0)
        return EVENT_ERROR;

    /* 버튼을 2초 이상 누르면 한 번만 OFF 요청 */
    if (sample.pressed) {
        if (!key_held) {
            key_held = 1;
            long_sent = 0;
            press_started = now;
        }

        if (!long_sent &&
            elapsed_ms(&now) >= FAN_LONG_PRESS_MS) {
            long_sent = 1;
            return EVENT_LONG_PRESS;
        }
    } else {
        key_held = 0;
        long_sent = 0;
    }

    /* 초음파 거리 읽기: mm 단위 */
    n = read(sensor_fd, &distance_mm,
             sizeof(distance_mm));

    if (n != (ssize_t)sizeof(distance_mm)) {
        if (n < 0 &&
            (errno == ETIMEDOUT || errno == EBUSY)) {
            near_sensor = 0;
            return EVENT_SENSOR_LOST;
        }

        if (n >= 0)
            errno = EIO;

        return EVENT_ERROR;
    }

    printf("거리: %.1f cm\n", distance_mm / 10.0);
    fflush(stdout);

    /* 유효하지 않은 측정값이면 정지 요청 */
    if (distance_mm <= 0 || distance_mm > 4000) {
        near_sensor = 0;
        return EVENT_SENSOR_LOST;
    }

    /* 20cm 초과: 정지 및 재접근 준비 */
    if (distance_mm > 200) {
        near_sensor = 0;
        return EVENT_FAR;
    }

    /* 처음 20cm 이하로 접근하면 S1 시작 요청 */
    if (!near_sensor) {
        near_sensor = 1;
        return EVENT_NEAR;
    }

    /* 버튼을 누르고 있는 동안 회전 입력 무시 */
    if (sample.pressed)
        return EVENT_NONE;

    /* 이전 회전량을 쌓지 않고 현재 방향만 전달 */
    if (sample.steps > 0)
        return EVENT_SPEED_UP;

    if (sample.steps < 0)
        return EVENT_SPEED_DOWN;

    return EVENT_NONE;
}