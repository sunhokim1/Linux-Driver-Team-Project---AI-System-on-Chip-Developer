# Project

## Type
임베디드 시스템 / Linux Character Device Driver 학습용 팀 프로젝트 (Smart Fan).

## Goal
NVIDIA Jetson Orin Nano에서 엔코더/초음파 입력으로 OFF/S1/S2/S3을 제어한다.
커널 드라이버와 사용자 공간의 통신 및 모듈별 독립 개발을 학습한다.
사용자 프로그램과 커널 드라이버 코드가 구현되어 있으며 실제 보드 검증은 별도다.

## Primary users
Linux 드라이버를 개발하는 팀원 3명과 보드에서 Smart Fan을 사용하는 사용자.

## Tech stack
C (사용자 코드: C11), GCC, GNU Make, Kbuild, Linux GPIO/IRQ 및 PWM Framework.
Character Device의 open/read/write/close로 통신한다. ioctl은 확장용 자리만 존재한다.

## Runtime / platform
타깃: NVIDIA Jetson Orin Nano / Jetson Linux.
캐리어 보드: NVIDIA 정품 Orin Nano Developer Kit. JetPack/커널 버전과 PWM ID는 보드에서 확인한다.
현재 작업 환경: Windows PowerShell. Makefile 실행은 Linux / Jetson 환경을 기준으로 한다.

## Architecture
입력: `kernel/ultrasonic/` -> `/dev/fan_encoder`, `/dev/ultrasonic` -> `user/event/`.
기본은 엔코더 전용이며 `--auto`에서만 초음파를 읽는다.
판단: `user/state/`가 이벤트에 따라 전원 및 풍량 상태를 관리한다.
출력: `user/output/` -> `/dev/fan_pwm` -> `kernel/fan_pwm/` -> 모터. LED 통합은 미구현이다.
통합 진입점은 `user/main.c`, 공통 인터페이스는 `include/`이다.
센서와 출력 페이로드는 native int이며 단위는 mm와 논리 출력 0/2/5/8이다.

## Design
GUI는 정의되어 있지 않다. 상태 전환 초안은 `docs/state_diagram.md`,
하드웨어 협의 사항은 `docs/hardware.md`, 통신 규격은 `docs/driver_interface.md`를 따른다.

## Verification
`Makefile`에 `make user`, `make test-build`, `make test`, `make kernel`이 정의되어 있다.
표준 명령은 `.ai/verification.yaml`에 등록한다. 커널 빌드는 타깃과 일치하는 KDIR이 필요하다.
State/Event/Output 테스트와 Main의 11개 시나리오는 모의 입력/출력으로 검증한다.
커널 빌드와 GPIO/PWM 측정, 실제 모터/LED 검증은 타깃에서 별도로 수행한다.

## Constraints
- 기존 스켈레톤, 공통 인터페이스와 팀별 담당 구조를 보존한다.
- Member 1: 초음파 드라이버 / Event. Member 2: State / Main / 통합. Member 3: PWM / Output.
- mode가 기준이며 OFF=0, S1=2, S2=5, S3=8로 power/speed를 함께 계산한다.
- 엔코더 버튼은 짧게 떼면 ON/OFF, 2초 이상이면 OFF다. 회전은 단계 끝에서 멈춘다.
- 자동 모드는 20cm 이내 시작/밖 정지, 버튼 debounce 30ms, 현재 PWM duty 0/60/80/100%다.
- 5V Echo는 GPIO에 맞게 레벨 변환하며 모터는 별도 전원과 드라이버 회로를 사용한다.
- 프레임워크나 API를 설치/연결하지 않으며 스타터 선택 기능은 비활성으로 유지한다.

확인 근거: `README.md`, 루트 및 커널 `Makefile`, `include/fan_common.h`,
`user/main.c`, `tests/test_state.c`, `docs/`.
