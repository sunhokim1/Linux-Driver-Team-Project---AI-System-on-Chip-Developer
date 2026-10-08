# Smart Fan — Linux Device Driver Project

## 1. 프로젝트 개요

### 프로젝트 목표

NVIDIA Jetson Orin Nano 환경에서 **Linux Kernel Device Driver를 직접 구현**하여 초음파 센서 입력에 따라 선풍기의 동작 상태와 풍량을 제어하는 임베디드 시스템을 개발한다.

본 프로젝트는 단순한 하드웨어 제어가 아닌, **Linux Character Device Driver 구현과 Kernel Space / User Space 간 통신**에 중점을 둔다.

### 핵심 학습 목표

- Linux Kernel Module 개발
- Character Device Driver 구현
- `file_operations` 구조체 활용
- GPIO 제어 및 인터럽트 처리
- Linux PWM Framework 활용
- `read()`, `write()`, `ioctl()` 등 시스템 콜 이해
- Kernel Space와 User Space 간 데이터 전달
- Event 기반 State Machine 설계
- 모듈별 독립 개발 및 통합

### 주요 기능

- 초음파 센서를 통한 거리 측정
- 거리 입력에 따른 이벤트 생성
- 선풍기 ON/OFF 제어
- 풍량 1~8단계 제어
- PWM 기반 모터 속도 제어
- LED를 통한 풍량 상태 표시
- Character Device를 이용한 사용자 프로그램과 커널 드라이버 간 통신

---

## 2. 개발 환경

| 항목 | 내용 |
|---|---|
| Target Board | NVIDIA Jetson Orin Nano |
| OS | Jetson Linux (JetPack 버전 확인 예정) |
| Language | C |
| Kernel | JetPack 설치 버전에 따라 확정 |
| Build | GCC, Make, Kbuild |
| Hardware | 초음파 센서, DC 모터, 모터 드라이버, PWM LED |
| Communication | Character Device (`/dev`) |
| Version Control | Git / GitHub |

> Jetson Orin Nano의 PWM 핀 및 GPIO Pinmux는 사용하는 캐리어 보드와 JetPack 버전에 맞게 설정한다.

---

## 3. 전체 시스템 아키텍처

```text
                  [USER SPACE]

+-------------------------------------------+
|                  main.c                   |
|                                           |
|  +-------------------------------------+  |
|  |          Event Handler              |  |
|  | - 거리값에 따른 이벤트 생성          |  |
|  +------------------+------------------+  |
|                     |                     |
|                     v                     |
|  +-------------------------------------+  |
|  |          State Machine              |  |
|  | - ON / OFF 상태 관리                |  |
|  | - 풍량 0~8단계 관리                 |  |
|  +------------------+------------------+  |
|                     |                     |
|                     v                     |
|  +-------------------------------------+  |
|  |          Output Controller          |  |
|  | - State에 따른 출력 명령 생성        |  |
|  +-------------------------------------+  |
+-------------------------------------------+
             |                   |
             | read()            | write()
             v                   v
============== KERNEL BOUNDARY ==============
             |                   |
             v                   v
+-------------------+  +--------------------+
| Ultrasonic Driver |  |   Fan PWM Driver   |
|                   |  |                    |
| /dev/ultrasonic   |  | /dev/fan_pwm       |
|                   |  |                    |
| - GPIO Control    |  | - PWM Framework    |
| - Echo Interrupt  |  | - Duty Control     |
| - Distance        |  | - Motor Control    |
|   Measurement     |  | - LED Control      |
+---------+---------+  +---------+----------+
          |                      |
          v                      v
 +----------------+    +--------------------+
 | Ultrasonic     |    | Motor Driver / LED |
 | Sensor         |    |                    |
 +----------------+    +--------------------+
```

### 설계 원칙

1. Kernel Space는 하드웨어 제어 및 데이터 전달을 담당한다.
2. User Space는 이벤트 판단, 상태 전환 및 실행 흐름을 담당한다.
3. Event 모듈은 State를 직접 변경하지 않는다.
4. State 모듈은 GPIO 또는 PWM 하드웨어를 직접 제어하지 않는다.
5. 사용자 프로그램은 Character Device 인터페이스를 통해 커널 드라이버와 통신한다.
6. 각 모듈은 독립적으로 구현하고 테스트할 수 있어야 한다.

---

## 4. 팀원별 역할 분담

### Member 1 — Ultrasonic Device Driver / Event

**역할: 센서 입력 및 이벤트 처리**

#### Kernel Space

- 초음파 센서 Character Device Driver 구현
- GPIO Trigger 신호 생성
- GPIO Echo 신호 감지
- Interrupt 기반 Echo 펄스 폭 측정
- 거리 계산
- `read()` 인터페이스 구현
- `/dev/ultrasonic` 디바이스 제공

#### User Space

- 초음파 거리값 읽기
- 거리 조건에 따른 Event 생성
- Debounce 및 중복 이벤트 방지
- 센서 오류 및 Timeout 처리

#### 담당 파일

```text
kernel/ultrasonic/
├── ultrasonic_drv.c
└── Makefile

user/event/
├── event_handler.c
└── event_handler.h
```

#### 핵심 인터페이스

```c
fan_event_t get_event(void);
```

#### 완료 조건

- [ ] Kernel Module 정상 로드 / 언로드
- [ ] GPIO Trigger / Echo 동작 확인
- [ ] 거리 측정값 획득
- [ ] `/dev/ultrasonic` 생성
- [ ] `read()`를 통한 거리값 전달
- [ ] 거리 조건에 따른 Event 생성
- [ ] 센서 Timeout 처리
- [ ] 중복 이벤트 방지

---

### Member 2 — State Machine / Main / Integration

**역할: 시스템 상태 관리 및 전체 통합**

#### User Space

- Event 기반 State Machine 구현
- 선풍기 전원 ON/OFF 상태 관리
- 풍량 0~8단계 상태 관리
- State Transition Table 설계
- Main Loop 구현
- Character Device 통신 코드 통합
- 전체 모듈 연결 및 테스트
- 오류 발생 시 안전 정지 처리

#### Kernel / Driver Interface

- Character Device 통신 규격 정의
- `read()` / `write()` 데이터 형식 협의
- 공통 헤더 관리
- 드라이버 통합 테스트

> Member 2는 주로 User Space와 통합을 담당한다. 모든 팀원이 커널 모듈을 직접 작성해야 하는 경우에는 별도의 커널 역할을 추가 배정한다.

#### 담당 파일

```text
include/
└── fan_common.h

user/state/
├── fan_state.c
└── fan_state.h

user/
└── main.c

tests/
└── test_state.c
```

#### 핵심 인터페이스

```c
void fan_state_init(fan_state_t *state);

bool handle_event(
    fan_state_t *state,
    fan_event_t event
);
```

#### 완료 조건

- [ ] 초기 상태 OFF 설정
- [ ] Event에 따른 State 전환
- [ ] 풍량 0~8단계 범위 제한
- [ ] State 변경 여부 반환
- [ ] Main Loop 구현
- [ ] Device Driver 인터페이스 통합
- [ ] State Unit Test
- [ ] 전체 시스템 통합 테스트

---

### Member 3 — PWM Device Driver / Motor / LED

**역할: PWM 기반 출력 제어**

#### Kernel Space

- Fan Character Device Driver 구현
- Linux PWM Framework 연동
- PWM 자원 획득 및 해제
- `write()` 인터페이스 구현
- 풍량 0~8단계 PWM 변환
- 모터 PWM 제어
- LED 상태 표시 제어
- `/dev/fan_pwm` 디바이스 제공

#### User Space

- Fan Driver 접근
- State에 따른 PWM 명령 전달
- 드라이버 오류 처리

#### 담당 파일

```text
kernel/fan_pwm/
├── fan_pwm_drv.c
└── Makefile

user/output/
├── fan_output.c
└── fan_output.h
```

#### 핵심 인터페이스

```c
int fan_output_init(void);

int apply_fan_state(
    const fan_state_t *state
);

void fan_output_cleanup(void);
```

#### 완료 조건

- [ ] PWM 출력 테스트
- [ ] Fan Kernel Module 정상 로드 / 언로드
- [ ] `/dev/fan_pwm` 생성
- [ ] `write()`를 통한 속도 명령 수신
- [ ] Linux PWM Framework 연동
- [ ] 풍량 1~8단계 출력
- [ ] LED 상태 표시
- [ ] OFF 시 모터 정지
- [ ] 장치 해제 및 오류 처리

---

## 5. 프로젝트 디렉터리 구조

```text
smart-fan/
│
├── README.md
├── Makefile
│
├── include/
│   ├── fan_common.h
│   └── fan_ioctl.h
│
├── kernel/
│   │
│   ├── ultrasonic/
│   │   ├── ultrasonic_drv.c
│   │   └── Makefile
│   │
│   └── fan_pwm/
│       ├── fan_pwm_drv.c
│       └── Makefile
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
│   ├── test_state.c
│   └── test_pwm.c
│
└── docs/
    ├── hardware.md
    ├── driver_interface.md
    └── state_diagram.md
```

---

## 6. Kernel / User Space 인터페이스

본 프로젝트는 Character Device Driver를 통해 사용자 프로그램과 커널 드라이버 간 통신을 수행한다.

### 디바이스 파일

| Device File | 기능 | System Call |
|---|---|---|
| `/dev/ultrasonic` | 거리값 획득 | `open()`, `read()`, `close()` |
| `/dev/fan_pwm` | 모터 속도 설정 | `open()`, `write()`, `close()` |

`/dev` 파일명은 프로젝트에서 정의하는 목표 인터페이스이며, 실제 등록 및 생성 코드는 커널 모듈에서 구현한다.

### Ultrasonic Driver Interface

```c
int fd = open("/dev/ultrasonic", O_RDONLY);

int distance_mm;

read(fd, &distance_mm, sizeof(distance_mm));

close(fd);
```

거리값은 `int` 타입의 밀리미터 단위로 전달하는 것을 기본 규격으로 한다.

### Fan PWM Driver Interface

```c
int fd = open("/dev/fan_pwm", O_WRONLY);

int speed = 4;

write(fd, &speed, sizeof(speed));

close(fd);
```

풍량은 `int` 타입의 0~8 값으로 전달한다.

### 커널 드라이버 주요 구조

```c
static const struct file_operations fan_fops = {
    .owner   = THIS_MODULE,
    .open    = fan_open,
    .write   = fan_write,
    .release = fan_release,
};
```

### PWM 제어 흐름

```text
User Program
     |
     | write(fd, &speed, sizeof(speed))
     v
fan_write()
     |
     | copy_from_user()
     v
Speed Validation (0~8)
     |
     v
PWM Duty Calculation
     |
     v
pwm_apply_might_sleep()
     |
     v
Linux PWM Controller Driver
     |
     v
Jetson PWM Output Pin
     |
     v
Motor Driver Circuit
     |
     v
DC Motor
```

> Jetson의 기존 PWM Controller Driver를 재작성하는 것이 아니라, Linux PWM Framework를 활용하는 프로젝트 전용 Fan Character Device Driver를 구현한다.

---

## 7. 공통 데이터 구조

**파일: `include/fan_common.h`**

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
    int speed;
} fan_state_t;

#endif
```

### State 규칙

- `power == false` : 선풍기 OFF
- `power == true` : 선풍기 ON
- `speed == 0` : 출력 정지
- `speed == 1~8` : 풍량 단계
- OFF 상태에서는 속도 0 유지
- ON 상태에서는 속도 1~8 유지

---

## 8. Event Module Skeleton

**파일: `user/event/event_handler.c`**

```c
#include "event_handler.h"

fan_event_t get_event(void)
{
    // TODO 1: /dev/ultrasonic에서 거리 읽기

    // TODO 2: 거리값 유효성 검사

    // TODO 3: 거리 조건에 따른 이벤트 생성

    // TODO 4: Debounce 처리

    return EVENT_NONE;
}
```

### Event 정의 (초안)

| 입력 조건 | Event | 동작 |
|---|---|---|
| 손을 가까이 접근 | EVENT_SPEED_UP | 풍량 증가 |
| 다른 거리 구간 접근 | EVENT_SPEED_DOWN | 풍량 감소 |
| 손을 일정 시간 유지 | EVENT_POWER_TOGGLE | 전원 ON/OFF |
| 입력 없음 | EVENT_NONE | 상태 유지 |

거리 기준과 이벤트 우선순위는 실제 센서 테스트 후 결정한다.

---

## 9. State Machine Skeleton

**파일: `user/state/fan_state.c`**

```c
#include "fan_state.h"

void fan_state_init(fan_state_t *state)
{
    state->power = false;
    state->speed = 0;
}

bool handle_event(
    fan_state_t *state,
    fan_event_t event
)
{
    // TODO 1: 현재 State 확인

    // TODO 2: Event에 따른 상태 변경

    // TODO 3: Speed 범위 검사

    // TODO 4: State 변경 여부 반환

    return false;
}
```

### State Transition Table

| 현재 상태 | Event | 다음 상태 |
|---|---|---|
| OFF | POWER_TOGGLE | ON / 1단계 |
| OFF | SPEED_UP | OFF 유지 |
| OFF | SPEED_DOWN | OFF 유지 |
| ON / 1~7 | SPEED_UP | 현재 단계 +1 |
| ON / 8 | SPEED_UP | 8단계 유지 |
| ON / 2~8 | SPEED_DOWN | 현재 단계 -1 |
| ON / 1 | SPEED_DOWN | 1단계 유지 |
| ON | POWER_TOGGLE | OFF / 0단계 |
| 모든 상태 | EVENT_NONE | 상태 유지 |

---

## 10. PWM Device Driver Skeleton

**파일: `kernel/fan_pwm/fan_pwm_drv.c`**

```c
static ssize_t fan_write(
    struct file *file,
    const char __user *buf,
    size_t count,
    loff_t *ppos
)
{
    int speed;

    // TODO 1: count 크기 검증

    // TODO 2: copy_from_user()

    // TODO 3: speed 0~8 검증

    // TODO 4: PWM Duty 계산

    // TODO 5: pwm_apply_might_sleep()

    // TODO 6: 결과 및 오류 반환

    return sizeof(speed);
}
```

> 위 코드는 핵심 함수 스켈레톤이며 단독 빌드 가능한 전체 커널 모듈은 아니다. 실제 구현에는 헤더, PWM 자원 관리, 디바이스 등록 및 해제, 동기화와 오류 처리가 필요하다.

### PWM 단계 정의

| Speed | Duty Cycle 예시 |
|---|---|
| 0 | 0% (OFF) |
| 1 | 12.5% |
| 2 | 25% |
| 3 | 37.5% |
| 4 | 50% |
| 5 | 62.5% |
| 6 | 75% |
| 7 | 87.5% |
| 8 | 100% |

실제 모터의 최소 기동 Duty와 특성에 따라 PWM 매핑은 변경할 수 있다.

---

## 11. Main Loop Skeleton

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

    // TODO: 센서 장치 초기화
    // TODO: 초기 OFF 출력 적용
    // TODO: 종료 신호 처리

    while (1) {

        event = get_event();

        bool changed = handle_event(
            &state,
            event
        );

        if (changed) {
            if (apply_fan_state(&state) < 0) {
                // TODO: 오류 처리
                break;
            }
        }

        // TODO: 센서 측정 주기 제어
    }

    fan_output_cleanup();

    return 0;
}
```

---

## 12. Jetson Orin Nano 하드웨어 설계

### 초음파 센서

```text
Jetson GPIO (Trigger)
        |
        v
Ultrasonic Sensor
        |
        | Echo
        v
Voltage Level Shifter
        |
        v
Jetson GPIO (Echo)
```

초음파 센서가 5V Echo를 출력하는 경우 Jetson의 3.3V GPIO에 직접 연결하지 않고 레벨 변환 회로를 사용한다.

### 모터 제어

```text
Jetson PWM GPIO
        |
        v
Motor Driver / MOSFET Circuit
        |
        v
DC Motor
```

- 모터는 Jetson GPIO에서 직접 구동하지 않는다.
- 모터에 적합한 별도 전원을 사용한다.
- PWM 제어 방식은 모터 종류에 따라 결정한다.
- 모터 드라이버와 Jetson의 GND를 적절히 공통 연결한다.
- 역기전력 및 전원 노이즈에 대한 보호를 고려한다.

### PWM LED

```text
Jetson PWM Output
        |
        v
LED Drive Circuit
        |
        v
LED
```

LED와 모터를 독립적으로 제어하려면 각각의 PWM 출력 채널 또는 별도 출력 제어 방식이 필요할 수 있다.

### 개발 전 확인 사항

- [ ] JetPack / Kernel 버전 확인
- [ ] Jetson Orin Nano 캐리어 보드 확인
- [ ] GPIO Pinmux 확인
- [ ] PWM 채널 확인
- [ ] 초음파 센서 모델 확인
- [ ] 모터 모델 및 동작 전압 확인
- [ ] 모터 드라이버 회로 결정
- [ ] LED 연결 방식 결정

---

## 13. 개발 로드맵

### Phase 1 — 개발 환경 및 인터페이스 설계

- [ ] Jetson Orin Nano 개발 환경 구축
- [ ] Linux Kernel Headers 확인
- [ ] Kernel Module 빌드 테스트
- [ ] 공통 헤더 정의
- [ ] Character Device 인터페이스 확정
- [ ] GPIO / PWM 하드웨어 점검

### Phase 2 — 드라이버 개별 구현

**Member 1**

- [ ] GPIO 출력 구현
- [ ] Echo Interrupt 처리
- [ ] 거리 측정 로직 구현
- [ ] Character Device `read()` 구현

**Member 2**

- [ ] State Machine 구현
- [ ] Character Device 통신 테스트 프로그램 작성
- [ ] Main Loop 구현
- [ ] State Unit Test 작성

**Member 3**

- [ ] PWM 출력 테스트
- [ ] PWM Framework 연동
- [ ] Character Device `write()` 구현
- [ ] 모터 속도 제어
- [ ] LED 출력 제어

### Phase 3 — 모듈 통합

- [ ] Ultrasonic Driver → Event 연결
- [ ] Event → State 연결
- [ ] State → Fan PWM Driver 연결
- [ ] 모터 및 LED 실제 출력 확인
- [ ] 입력 이벤트 안정화
- [ ] 에러 처리 및 안전 정지 구현

### Phase 4 — 최종 테스트

- [ ] Kernel Module 반복 로드 / 언로드
- [ ] 센서 측정 테스트
- [ ] PWM 8단계 출력 테스트
- [ ] State Transition 테스트
- [ ] 전체 통합 테스트
- [ ] 코드 리뷰 및 문서 정리

---

## 14. 테스트 계획

### Kernel Driver Test

- [ ] `insmod` 성공
- [ ] `rmmod` 성공
- [ ] `/dev` 디바이스 생성 확인
- [ ] `open()` 동작 확인
- [ ] `read()` 정상 / 오류 동작 확인
- [ ] `write()` 정상 / 오류 동작 확인
- [ ] 잘못된 데이터 입력 처리
- [ ] 리소스 정상 해제
- [ ] 커널 로그 및 오류 확인

### State Machine Test

- [ ] OFF → ON 전환
- [ ] ON → OFF 전환
- [ ] 풍량 증가
- [ ] 풍량 감소
- [ ] 최대 풍량 제한
- [ ] 최소 풍량 제한
- [ ] EVENT_NONE 상태 유지

### Hardware Integration Test

- [ ] 초음파 거리 측정
- [ ] 이벤트 감지
- [ ] PWM 출력 측정
- [ ] 모터 8단계 제어
- [ ] LED 상태 표시
- [ ] 안전 정지 및 종료 처리

---

## 15. GitHub 협업 규칙

### Branch Strategy

```text
main
│
├── feature/ultrasonic-driver
│
├── feature/state-machine
│
└── feature/fan-pwm-driver
```

### Collaboration Rules

1. 각 팀원은 담당 Feature Branch에서 개발한다.
2. `main`에 직접 Push하지 않는다.
3. 공통 인터페이스 변경 시 팀원들과 협의한다.
4. 각 기능은 개별 테스트 후 Pull Request를 생성한다.
5. 코드 리뷰 후 Merge한다.
6. 커널 드라이버 변경 시 테스트 환경과 실행 결과를 PR에 기록한다.

### Commit Convention

```text
feat: 기능 추가
fix: 버그 수정
refactor: 코드 구조 개선
test: 테스트 코드 추가
docs: 문서 수정
build: 빌드 설정 수정
```

### Commit Examples

```text
feat: implement ultrasonic character driver
feat: add echo interrupt handling
feat: implement fan state machine
feat: implement pwm character driver
feat: add motor speed control
test: add driver interface tests
docs: update hardware architecture
```

---

## 16. 최종 목표

본 프로젝트는 Linux Device Driver를 직접 구현하고, 이를 기반으로 초음파 센서를 통한 이벤트 감지와 PWM 기반 선풍기 제어를 수행하는 것을 목표로 한다.

### 최종 구현 구성

```text
[Ultrasonic Sensor]
         |
         v
[Ultrasonic Kernel Driver]
         |
         | read()
         v
[Event Handler]
         |
         v
[State Machine]
         |
         v
[Output Controller]
         |
         | write()
         v
[Fan PWM Kernel Driver]
         |
         v
[Linux PWM Framework]
         |
         v
[Motor Driver / LED]
         |
         v
[Fan Speed Control]
```

### 프로젝트 핵심

**직접 구현할 Linux Device Driver**

1. Ultrasonic Character Device Driver
2. Fan PWM Character Device Driver

**사용자 공간 소프트웨어**

1. Event Handler
2. State Machine
3. Output Controller
4. Main Application

최종적으로 **Kernel Space와 User Space를 분리하고, Character Device Driver를 통해 하드웨어를 제어하는 이벤트 기반 임베디드 시스템**을 완성한다.
