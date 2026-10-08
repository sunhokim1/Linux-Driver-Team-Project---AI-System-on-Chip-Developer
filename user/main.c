#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "fan_common.h"
#include "event/event_handler.h"
#include "state/fan_state.h"
#include "output/fan_output.h"

static volatile sig_atomic_t stop_requested;

static void request_stop(int signal_number)
{
    (void)signal_number;
    /* No device I/O or logging in the signal handler. */
    stop_requested = 1;
}

static int install_signal_handlers(void)
{
#ifdef _WIN32
    /* Allows host-side tests; deployment uses the POSIX branch below. */
    if (signal(SIGINT, request_stop) == SIG_ERR ||
        signal(SIGTERM, request_stop) == SIG_ERR)
        return -1;
#else
    struct sigaction action = {0};

    action.sa_handler = request_stop;
    if (sigemptyset(&action.sa_mask) < 0 ||
        sigaction(SIGINT, &action, NULL) < 0 ||
        sigaction(SIGTERM, &action, NULL) < 0)
        return -1;
    /* No SA_RESTART: device reads should be able to return on shutdown. */
#endif
    return 0;
}

static int wait_for_next_measurement(void)
{
    /* 100 ms is a polling default, not a gesture/debounce threshold. */
    struct timespec remaining = { .tv_sec = 0, .tv_nsec = 100000000L };

    while (!stop_requested) {
        struct timespec interrupted;

        if (nanosleep(&remaining, &interrupted) == 0)
            return 0;
        if (errno != EINTR)
            return -1;
        remaining = interrupted;
    }
    return 0;
}

int main(void)
{
    fan_state_t state;
    int result = EXIT_FAILURE;

    fan_state_init(&state);

    if (install_signal_handlers() < 0) {
        perror("install_signal_handlers");
        return EXIT_FAILURE;
    }

    if (fan_output_init() < 0) {
        perror("fan_output_init");
        fan_output_cleanup();
        return EXIT_FAILURE;
    }

    if (apply_fan_state(&state) < 0) {
        perror("initial OFF output");
        goto shutdown;
    }

    /* Input descriptor lifetime belongs to Member 1's get_event().
     * It must return EVENT_ERROR for failures and use bounded waits so
     * a blocked read cannot prevent shutdown indefinitely.
     * Button duration/debounce belongs there too: SHORT on release <2 s,
     * LONG once at >=2 s, with no SHORT event after a LONG event.
     */
    result = EXIT_SUCCESS;
    while (!stop_requested) {
        fan_event_t event;

        errno = 0;
        event = get_event();
        if (stop_requested)
            break;

        if (event == EVENT_ERROR) {
            if (errno)
                perror("get_event");
            else
                fputs("get_event: input error\n", stderr);
            result = EXIT_FAILURE;
            break;
        }
        if (event < EVENT_NONE || event > EVENT_LONG_PRESS) {
            fputs("get_event: invalid event\n", stderr);
            result = EXIT_FAILURE;
            break;
        }

        if (handle_event(&state, event) && !stop_requested) {
            if (apply_fan_state(&state) < 0) {
                perror("apply_fan_state");
                result = EXIT_FAILURE;
                break;
            }
        }

        if (wait_for_next_measurement() < 0) {
            perror("measurement interval");
            result = EXIT_FAILURE;
            break;
        }
    }

shutdown:
    /* Try OFF even after a write failure; never claim the motor stopped
     * if this request fails. The output module owns resource cleanup.
     */
    fan_state_init(&state);
    if (apply_fan_state(&state) < 0) {
        perror("shutdown OFF output");
        result = EXIT_FAILURE;
    }
    fan_output_cleanup();
    return result;
}
