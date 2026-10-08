# Linux Device Driver 기반 스마트 선풍기 프로젝트

## 1. 프로젝트 개요

### 목표

Linux Device Driver를 활용하여 초음파 센서 입력에 따라 선풍기의 풍량을 제어하고, PWM 8단계 LED를 통해 현재 상태를 표시하는 임베디드 시스템을 구현한다.

### 주요 기능

- 초음파 센서를 통한 사용자 입력 감지
- 이벤트 기반 선풍기 상태 변경
- 풍량 0\~8단계 제어 (0: OFF)
- PWM 8단계 LED 출력
- Linux Device Driver를 통한 하드웨어 제어
- Event / State / Driver 모듈 분리

> 실제 모터 제어 방식, 초음파 입력 거리 조건, 사용 보드 및 GPIO 핀은 추후 확정한다.

---

## 2. 전체 시스템 아키텍처

```text
+-----------------------------+
|       Ultrasonic Sensor     |
+--------------+--------------+
               |
               v
+-----------------------------+
|       Event Module          |
|                             |
| - 초음파 거리 측정            |
| - 입력 조건 판별              |
| - 이벤트 생성                |
+--------------+--------------+
               |
               | fan_event_t
               v
+-----------------------------+
|       State Module          |
|                             |
| - 현재 선풍기 상태 관리       |
| - 이벤트에 따른 상태 변경     |
| - 풍량 0~8단계 관리          |
+--------------+--------------+
               |
               | fan_state_t
               v
+-----------------------------+
|       Output Module         |
|                             |
| - PWM Duty 계산              |
| - LED 출력 요청              |
| - 모터 제어 요청              |
+--------------+--------------+
               |
               v
+-----------------------------+
|      Linux Device Driver    |
|                             |
| - PWM Driver                |
| - LED Driver                |
| - Hardware Control          |
+-----------------------------+
```

### 설계 원칙

1. Event 모듈은 State를 직접 변경하지 않는다.
2. State 모듈은 하드웨어를 직접 제어하지 않는다.
3. Driver 모듈은 이벤트나 상태 전환 규칙을 알 필요가 없다.
4. 각 모듈은 공통 헤더에 정의된 인터페이스를 사용한다.
5. 상태 변경 및 하드웨어 출력은 독립적으로 테스트할 수 있어야 한다.

---

## 3. 팀원별 역할 분담

### Member 1 — Event / Ultrasonic 담당

**담당 범위**

- 초음파 센서 Linux Device Driver 구현
- Trigger / Echo GPIO 처리
- 거리 측정 및 값 전달
- 거리값을 이벤트로 변환
- 입력 중복 방지 및 예외 처리

**주요 파일**

```text
kernel/ultrasonic_drv.c
user/event/event_handler.c
user/event/event_handler.h
```

**제공할 인터페이스**

```c
fan_event_t get_event(void);
```

**완료 조건**

- 초음파 센서의 거리값을 읽을 수 있다.
- 정해진 거리 조건에 따라 이벤트가 발생한다.
- 손을 계속 대고 있을 때 이벤트가 무한 반복되지 않는다.
- 센서 측정 실패 시 잘못된 상태 변경이 발생하지 않는다.

### Member 2 — State / Main 담당

**담당 범위**

- 선풍기 State Machine 구현
- Event에 따른 State 전환
- 풍량 단계 및 전원 상태 관리
- Main Loop 구현
- 모듈 통합 및 전체 동작 테스트

**주요 파일**

```text
user/state/fan_state.c
user/state/fan_state.h
user/main.c
```

**제공할 인터페이스**

```c
void fan_state_init(fan_state_t *state);

bool handle_event(fan_state_t *state,
                  fan_event_t event);
```

**완료 조건**

- 모든 Event에 대한 상태 전환이 정의되어 있다.
- 풍량이 0\~8 범위를 벗어나지 않는다.
- 동일 이벤트에 대한 상태 전환 결과가 일관적이다.
- 실제 하드웨어 없이 상태 변경 단위 테스트가 가능하다.
- 세 모듈을 통합하여 동작시킬 수 있다.

### Member 3 — PWM / LED / Motor 담당

**담당 범위**

- PWM Linux Device Driver 구현 또는 Linux PWM Subsystem 연동
- LED 8단계 밝기 출력
- 모터 PWM 제어
- PWM Duty Cycle 계산
- State에 따른 하드웨어 출력

**주요 파일**

```text
kernel/pwm_drv.c
user/output/fan_output.c
user/output/fan_output.h
```

**제공할 인터페이스**

```c
int fan_output_init(void);

int apply_fan_state(const fan_state_t *state);

void fan_output_cleanup(void);
```

**완료 조건**

- PWM Duty Cycle 변경이 가능하다.
- LED가 8단계 밝기로 표시된다.
- State 변경 시 실제 출력이 변경된다.
- OFF 상태에서 PWM 출력이 중지된다.
- 하드웨어 오류를 상위 모듈에 전달할 수 있다.

---

## 4. 프로젝트 디렉터리 구조

```text
smart-fan/
│
├── README.md
├── IMPLEMENTATION_PLAN.md
├── Makefile
│
├── include/
│   └── fan_common.h
│
├── kernel/
│   ├── Makefile
│   ├── ultrasonic_drv.c
│   └── pwm_drv.c
│
├── user/
│   ├── main.c
│   │
│   ├── event/
│   │   ├── event_handler.c
│   │   └── event_handler.h
│   │
│   ├── state/
│   │   ├── fan_state.c
│   │   └── fan_state.h
│   │
│   └── output/
│       ├── fan_output.c
│       └── fan_output.h
│
├── tests/
│   ├── test_event.c
│   └── test_state.c
│
└── docs/
    ├── hardware.md
    └── interface.md
```

---

## 5. 공통 데이터 구조

**파일: `include/fan_common.h`**

모든 팀원은 이 헤더에 정의된 타입을 공유한다.

```c
#ifndef FAN_COMMON_H
#define FAN_COMMON_H

#include <stdbool.h>

typedef enum {
    EVENT_NONE = 0,
    EVENT_SPEED_UP,
    EVENT_SPEED_DOWN,
    EVENT_POWER_TOGGLE
} fan_event_t;

typedef struct {
    bool power;
    int speed;  // 0~8
} fan_state_t;

#endif
```

### 공통 규칙

- `power == false`이면 선풍기는 OFF 상태이다.
- `speed == 0`은 OFF 상태를 의미한다.
- `power == true`이면 `speed`는 1\~8이다.
- 이벤트 타입은 `fan_event_t`를 사용한다.
- 상태 데이터는 `fan_state_t`를 사용한다.
- State 변경은 State 모듈에서만 수행한다.

---

## 6. Event Module 구현 스켈레톤

**파일: `user/event/event_handler.c`**

```c
#include "event_handler.h"

fan_event_t get_event(void)
{
    // TODO 1: 초음파 센서 드라이버에서 거리 읽기

    // TODO 2: 유효한 측정값인지 확인

    // TODO 3: 거리 조건에 따른 이벤트 판별

    // TODO 4: Debounce / 재입력 방지

    return EVENT_NONE;
}
```

### 이벤트 정의 (초안)

| 조건              | 이벤트                | 동작    |
| --------------- | ------------------ | ----- |
| 손을 가까이 접근       | EVENT_SPEED_UP     | 풍량 증가 |
| 손을 다른 거리 구간에 접근 | EVENT_SPEED_DOWN   | 풍량 감소 |
| 손을 일정 시간 유지     | EVENT_POWER_TOGGLE | 전원 토글 |
| 입력 없음           | EVENT_NONE         | 상태 유지 |

거리 임계값과 입력 유지 시간은 센서 테스트 후 확정한다.

동일한 센서 입력이 여러 이벤트로 해석되지 않도록 우선순위를 정의한다.

---

## 7. State Module 구현 스켈레톤

**파일: `user/state/fan_state.c`**

```c
#include "fan_state.h"

void fan_state_init(fan_state_t *state)
{
    state->power = false;
    state->speed = 0;
}

bool handle_event(fan_state_t *state,
                  fan_event_t event)
{
    // TODO 1: 현재 State 확인

    // TODO 2: Event에 따른 State 변경

    // TODO 3: 풍량 범위 제한 (0~8)

    // TODO 4: 변경 여부 반환

    return false;
}
```

### State Transition Table

| 현재 상태       | 이벤트          | 다음 상태     |
| ----------- | ------------ | --------- |
| OFF         | SPEED_UP     | OFF 유지    |
| OFF         | SPEED_DOWN   | OFF 유지    |
| OFF         | POWER_TOGGLE | ON / 1단계  |
| ON / 1\~7단계 | SPEED_UP     | 현재 풍량 +1  |
| ON / 8단계    | SPEED_UP     | 8단계 유지    |
| ON / 2\~8단계 | SPEED_DOWN   | 현재 풍량 -1  |
| ON / 1단계    | SPEED_DOWN   | 1단계 유지    |
| ON          | POWER_TOGGLE | OFF / 0단계 |
| 모든 상태       | EVENT_NONE   | 상태 유지     |

> 위 표를 1차 상태 전환 규칙으로 사용한다. 전원 OFF 후 이전 풍량을 기억하는 기능은 추후 확장한다.

---

## 8. PWM / Output Module 구현 스켈레톤

**파일: `user/output/fan_output.c`**

```c
#include "fan_output.h"

int fan_output_init(void)
{
    // TODO: PWM / LED 장치 열기
    return 0;
}

int apply_fan_state(const fan_state_t *state)
{
    // TODO 1: 전원 상태 확인

    // TODO 2: speed -> PWM Duty 변환

    // TODO 3: 모터 PWM 설정

    // TODO 4: LED PWM 설정

    // TODO 5: 출력 성공 / 실패 반환

    return 0;
}

void fan_output_cleanup(void)
{
    // TODO: PWM OFF 및 장치 닫기
}
```

### PWM 단계 정의

| Speed   | PWM Duty (예시) |
| ------- | ------------- |
| 0 (OFF) | 0%            |
| 1       | 12.5%         |
| 2       | 25%           |
| 3       | 37.5%         |
| 4       | 50%           |
| 5       | 62.5%         |
| 6       | 75%           |
| 7       | 87.5%         |
| 8       | 100%          |

LED PWM과 모터 PWM은 하드웨어에 따라 각각 다른 Duty 매핑을 사용할 수 있다.

특히 모터는 최소 기동 Duty와 드라이버 회로 특성을 고려해야 한다.

---

## 9. Main Loop 구현 스켈레톤

**파일: `user/main.c`**

```c
#include "fan_common.h"
#include "event/event_handler.h"
#include "state/fan_state.h"
#include "output/fan_output.h"

int main(void)
{
    fan_state_t state;
    fan_event_t event;

    fan_state_init(&state);

    if (fan_output_init() < 0)
        return 1;

    // TODO: 센서 입력 초기화
    // TODO: 종료 신호 처리

    while (1) {

        // 1. Event 읽기
        event = get_event();

        // 2. State 변경
        bool changed = handle_event(&state, event);

        // 3. State 변경 시 출력 반영
        if (changed) {
            if (apply_fan_state(&state) < 0) {
                // TODO: 안전 정지 및 오류 처리
                break;
            }
        }

        // TODO: 측정 주기 제어
    }

    fan_output_cleanup();

    return 0;
}
```

초기 출력 OFF 설정, 장치 열기 실패, 센서 오류, 프로그램 종료 시 안전 정지 처리도 통합 단계에서 구현한다.

---

## 10. Linux Kernel / User Space 경계

| 기능                     | 실행 위치                      |
| ---------------------- | -------------------------- |
| GPIO Trigger / Echo 제어 | Kernel                     |
| 초음파 거리 측정              | Kernel 또는 User (설계에 따라 결정) |
| 거리값 이벤트 변환             | User                       |
| State Machine          | User                       |
| PWM 하드웨어 설정            | Kernel                     |
| LED / 모터 출력 명령         | User → Kernel              |

### 디바이스 인터페이스 (예정)

```text
/dev/ultrasonic
/dev/fan_pwm
```

예상 통신 방식:

```text
User Space
    |
    | read() / write() / ioctl()
    v
Device File
    |
    v
Linux Kernel Driver
    |
    v
GPIO / PWM Hardware
```

디바이스 이름, IOCTL 명령 및 데이터 구조는 커널 드라이버 담당자와 협의 후 확정한다.

---

## 11. 팀원 간 인터페이스 계약

| API                  | 담당       | 입력            | 출력          |
| -------------------- | -------- | ------------- | ----------- |
| get_event()          | Member 1 | 없음            | fan_event_t |
| fan_state_init()     | Member 2 | fan_state_t\* | void        |
| handle_event()       | Member 2 | State, Event  | 변경 여부(bool) |
| fan_output_init()    | Member 3 | 없음            | 성공/실패       |
| apply_fan_state()    | Member 3 | fan_state_t\* | 성공/실패       |
| fan_output_cleanup() | Member 3 | 없음            | void        |

**핵심 규칙**

- 공통 헤더 변경 시 팀원과 협의한다.
- 다른 사람의 모듈 내부 구현을 직접 참조하지 않는다.
- 모듈 간 호출은 합의된 함수만 사용한다.
- 하드웨어가 없어도 Mock 데이터를 통해 테스트한다.

---

## 12. 테스트 계획

### Event 테스트

- [ ] 초음파 거리 측정 성공
- [ ] 유효하지 않은 거리값 처리
- [ ] SPEED_UP 이벤트 생성
- [ ] SPEED_DOWN 이벤트 생성
- [ ] POWER_TOGGLE 이벤트 생성
- [ ] 연속 입력 중복 방지

### State 테스트

- [ ] 초기 상태 OFF / 0단계 확인
- [ ] 전원 ON 시 1단계 진입
- [ ] 풍량 증가 / 감소 확인
- [ ] 최대 8단계 초과 방지
- [ ] 최소 1단계 미만 감소 방지
- [ ] OFF 상태에서 풍량 변경 무시
- [ ] EVENT_NONE 시 상태 유지

### Output 테스트

- [ ] PWM 출력 성공
- [ ] LED 8단계 표시
- [ ] OFF 시 출력 중지
- [ ] 모터 기동 / 정지 확인
- [ ] 장치 접근 실패 처리

### 통합 테스트

- [ ] 초음파 입력 → 이벤트 발생
- [ ] 이벤트 발생 → State 변경
- [ ] State 변경 → PWM 반영
- [ ] LED와 실제 상태 일치
- [ ] 프로그램 종료 시 출력 정지

---

## 13. 개발 진행 순서

### Phase 1 — 인터페이스 및 환경 설정

- [ ] 사용 보드 및 핀맵 확정
- [ ] `fan_common.h` 정의
- [ ] 함수 인터페이스 확정
- [ ] 프로젝트 디렉터리 생성
- [ ] 커널 모듈 빌드 환경 설정

### Phase 2 — 개별 모듈 구현

**Member 1**

- [ ] 초음파 드라이버 작성
- [ ] 거리 측정 테스트
- [ ] 이벤트 변환 구현

**Member 2**

- [ ] State Machine 구현
- [ ] State 단위 테스트
- [ ] Main Loop 작성

**Member 3**

- [ ] PWM 드라이버 구현
- [ ] LED 출력 테스트
- [ ] 모터 제어 테스트

### Phase 3 — 통합

- [ ] Event → State 연결
- [ ] State → Output 연결
- [ ] 실제 하드웨어 통합
- [ ] 타이밍 / 입력 안정화
- [ ] 오류 처리 및 안전 정지

### Phase 4 — 최종 검증

- [ ] 전체 기능 테스트
- [ ] 코드 리뷰
- [ ] 버그 수정
- [ ] README 작성
- [ ] 시연 준비

---

## 14. GitHub 협업 규칙

### 브랜치 구조

```text
main
├── feature/event
├── feature/state
└── feature/pwm-output
```

### 작업 규칙

1. 각자 담당 브랜치에서 작업한다.
2. `main` 브랜치에 직접 Push하지 않는다.
3. 기능 구현 후 Pull Request를 생성한다.
4. 공통 인터페이스 변경은 PR에서 팀원 리뷰를 받는다.
5. 통합 테스트 통과 후 `main`에 Merge한다.

### Commit Convention

```text
feat: 기능 추가
fix: 오류 수정
refactor: 코드 구조 개선
test: 테스트 추가
docs: 문서 변경
```

### Commit 예시

```text
feat: add ultrasonic distance measurement
feat: implement fan state transitions
feat: add 8-level pwm control
test: add state transition tests
docs: update implementation plan
```

---

## 15. 최종 구현 목표

```text
[초음파 센서]
      |
      v
[Event 감지]
      |
      v
[Event 전달]
      |
      v
[State Machine]
      |
      v
[풍량 State 변경]
      |
      v
[PWM 출력 제어]
      |
      +--------> [LED 상태 표시]
      |
      +--------> [선풍기 모터 제어]
```

**최종 목표: Event, State, Driver를 독립적인 모듈로 개발하고, 명확한 인터페이스를 통해 통합 가능한 Linux Device Driver 기반 스마트 선풍기 시스템 구현.**
