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

static void stop_handler(int signo)
{
    (void)signo;
    stop_requested = 1;
}

static void print_state(const fan_state_t *state)
{
    const char *name;

    switch (state->mode) {
    case FAN_MODE_S1:
        name = "S1";
        break;

    case FAN_MODE_S2:
        name = "S2";
        break;

    case FAN_MODE_S3:
        name = "S3";
        break;

    default:
        name = "OFF";
        break;
    }

    printf("팬 상태: %s / 상태 값: %d\n",
           name, state->speed);
    fflush(stdout);
}

int main(void)
{
    struct sigaction action = {0};

    /* 초음파 드라이버에서 이미 측정 간격을 확보함 */
    const struct timespec interval = {
        .tv_sec = 0,
        .tv_nsec = 10000000L
    };

    fan_state_t state;
    int result = EXIT_FAILURE;

    action.sa_handler = stop_handler;

    if (sigemptyset(&action.sa_mask) < 0 ||
        sigaction(SIGINT, &action, NULL) < 0 ||
        sigaction(SIGTERM, &action, NULL) < 0) {
        perror("종료 신호 설정 실패");
        return EXIT_FAILURE;
    }

    fan_state_init(&state);

    if (fan_output_init() < 0) {
        perror("모터 장치 초기화 실패");
        return EXIT_FAILURE;
    }

    if (apply_fan_state(&state) < 0) {
        perror("초기 정지 출력 실패");
        goto shutdown;
    }

    puts("20cm 이하 접근: S1 시작");
    puts("엔코더 오른쪽: S1 → S2 → S3");
    puts("엔코더 왼쪽: S3 → S2 → S1");
    puts("엔코더 버튼 2초 누름: OFF");
    puts("20cm 초과: OFF");
    puts("버튼으로 정지한 뒤에는 멀어졌다가 다시 접근하세요.");
    puts("종료: Ctrl+C");

    print_state(&state);

    result = EXIT_SUCCESS;

    while (!stop_requested) {
        fan_event_t event = get_event();

        if (stop_requested)
            break;

        if (event == EVENT_ERROR) {
            perror("초음파/엔코더 입력 실패");
            result = EXIT_FAILURE;
            break;
        }

        if (event < EVENT_NONE || event > EVENT_SENSOR_LOST) {
            fprintf(stderr, "유효하지 않은 이벤트: %d\n",
                    (int)event);
            result = EXIT_FAILURE;
            break;
        }

        if (handle_event(&state, event)) {
            if (stop_requested)
                break;

            if (apply_fan_state(&state) < 0) {
                perror("팬 PWM 출력 실패");
                result = EXIT_FAILURE;
                break;
            }
        }

            print_state(&state);
        

        if (nanosleep(&interval, NULL) < 0 &&
            errno != EINTR) {
            perror("대기 실패");
            result = EXIT_FAILURE;
            break;
        }
    }

shutdown:
    fan_state_init(&state);

    if (apply_fan_state(&state) < 0) {
        perror("모터 정지 요청 실패");
        result = EXIT_FAILURE;
    }

    fan_output_cleanup();

    puts("팬 제어 종료");
    return result;
}