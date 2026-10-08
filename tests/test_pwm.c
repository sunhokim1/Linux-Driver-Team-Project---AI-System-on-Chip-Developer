#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
static int fake_open(const char *path, int flags, ...);
static int fake_close(int fd);
static ssize_t fake_write(int fd, const void *buf, size_t count);
#define open fake_open
#define close fake_close
#define write fake_write
#include "../user/output/fan_output.c"
#undef open
#undef close
#undef write

static int last_speed, writes, closes, failure;
static int fake_open(const char *path, int flags, ...)
{
    assert(strcmp(path, FAN_PWM_DEVICE_PATH) == 0);
    (void)flags;
    return 12;
}
static int fake_close(int fd) { assert(fd == 12); ++closes; return 0; }
static ssize_t fake_write(int fd, const void *buf, size_t count)
{
    assert(fd == 12 && count == sizeof(int));
    ++writes;
    memcpy(&last_speed, buf, count);
    if (failure == EINTR) { failure = 0; errno = EINTR; return -1; }
    if (failure == EIO) { errno = EIO; return -1; }
    return failure ? 1 : (ssize_t)count;
}
int main(void)
{
    fan_state_t state = {0};
    const int speeds[] = {0, 2, 5, 8};
    assert(fan_output_init() == 0 && last_speed == 0);
    assert(fan_output_init() == -1 && errno == EBUSY);
    for (int i = 0; i < 4; ++i) {
        state.mode = (fan_mode_t)i;
        state.speed = speeds[i]; state.power = i != 0;
        assert(apply_fan_state(&state) == 0 && last_speed == speeds[i]);
    }
    int before = writes;
    state.speed = 7;
    assert(apply_fan_state(&state) == -1 && errno == EINVAL && writes == before);
    state.speed = 8; state.power = false;
    assert(apply_fan_state(&state) == -1 && writes == before);
    state.power = true;
    failure = EINTR;
    assert(apply_fan_state(&state) == 0 && writes == before + 2);
    failure = 1;
    assert(apply_fan_state(&state) == -1 && errno == EIO);
    failure = EIO;
    assert(apply_fan_state(&state) == -1 && errno == EIO);
    fan_output_cleanup(); fan_output_cleanup();
    assert(closes == 1);
    assert(apply_fan_state(&state) == -1 && errno == EINVAL);
    assert(fan_output_init() == -1 && closes == 2);
    puts("PASS: output protocol 0/2/5/8, invalid states, short writes and cleanup");
    return 0;
}
