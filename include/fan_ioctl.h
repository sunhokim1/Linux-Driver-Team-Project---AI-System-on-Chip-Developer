#ifndef FAN_IOCTL_H
#define FAN_IOCTL_H

/* TODO (team): agree on ioctl commands and payloads if needed.
 * The initial interface uses read()/write(); no ioctl ABI is defined yet.
 */
#include <linux/ioctl.h>
#include <linux/types.h>

#define FAN_IOCTL_MAGIC 'F'

/*
 * HC-SR04 Echo 펄스 시간 반환
 * 단위: 마이크로초(us)
 * 사용자 프로그램에서 cm = echo_us / 58.0으로 변환
 */
#define FAN_IOCTL_GET_ECHO_US \
    _IOR(FAN_IOCTL_MAGIC, 1, __u32)

#endif /* FAN_IOCTL_H */

