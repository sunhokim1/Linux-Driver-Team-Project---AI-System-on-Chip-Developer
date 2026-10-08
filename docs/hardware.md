# 하드웨어 설정 (작성 예정)

담당: Member 1 (센서), Member 3 (모터 / LED)

| 항목 | 확정 값 |
|---|---|
| JetPack / Kernel 버전 | TODO |
| 캐리어 보드 | TODO |
| 초음파 센서 모델 | TODO |
| Trigger / Echo GPIO 및 Pinmux | TODO |
| Echo 레벨 변환 회로 | TODO |
| 모터 / 전압 / 별도 전원 | TODO |
| 모터 드라이버 회로 | TODO |
| 모터 PWM 채널 / 주기 | TODO |
| LED 연결 방식 / PWM 채널 | TODO |

핀 번호와 PWM API는 보드 및 커널 버전을 확인한 뒤 확정한다.
5V Echo 센서는 GPIO 입력에 맞는 레벨 변환을 사용한다.
모터는 드라이버 회로와 별도 전원을 사용하며 공통 GND 및 보호 회로를 검토한다.

TODO: 배선도, Device Tree / Pinmux 설정, 측정 결과 기록.
