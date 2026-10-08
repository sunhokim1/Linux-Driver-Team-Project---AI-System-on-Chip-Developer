#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/pwm.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/atomic.h>
#include <linux/err.h>

#include "fan_common.h"

/* 현재 보드의 물리 29번 핀: PQ.05, GPIO 453 */
#define GPIO_IN1 453

/* PWM 주기 1ms, 주파수 1kHz */
#define PWM_PERIOD_NS 1000000U

/* PWM 번호는 insmod 실행 시 전달 */
static int pwm_id = -1;
module_param(pwm_id, int, 0444);
MODULE_PARM_DESC(pwm_id, "Motor PWM global ID");

static struct pwm_device *motor_pwm;
static DEFINE_MUTEX(fan_lock);
static atomic_t opened = ATOMIC_INIT(0);

/* fan_lock을 잡은 상태에서 호출 */
static int set_speed(int speed)
{
    struct pwm_state state;
    unsigned int duty_percent;
    int ret;

    switch (speed) {
    case 0:
        duty_percent = 0;
        break;
    case 1:
        duty_percent = 60;
        break;
    case 2:
        duty_percent = 80;
        break;
    case 3:
        duty_percent = 100;
        break;
    default:
        return -EINVAL;
    }

    /* PWM 설정 중에는 모터 구동 중지 */
    gpio_set_value(GPIO_IN1, 0);

    pwm_get_state(motor_pwm, &state);

    state.period = PWM_PERIOD_NS;
    state.duty_cycle =
        (PWM_PERIOD_NS * duty_percent) / 100;
    state.polarity = PWM_POLARITY_NORMAL;

    /* OFF도 활성 상태에서 듀티 0%로 설정 */
    state.enabled = true;

    ret = pwm_apply_state(motor_pwm, &state);
    if (ret)
        return ret;

    /* IN2는 GND에 고정되어 있음 */
    if (speed > 0)
        gpio_set_value(GPIO_IN1, 1);

    return 0;
}

static int fan_open(struct inode *inode, struct file *file)
{
    int ret;

    /* 한 번에 하나의 프로그램만 장치를 열 수 있음 */
    if (atomic_cmpxchg(&opened, 0, 1))
        return -EBUSY;

    ret = nonseekable_open(inode, file);
    if (ret)
        atomic_set(&opened, 0);

    return ret;
}

static ssize_t fan_write(struct file *file,
                         const char __user *buf,
                         size_t count, loff_t *ppos)
{
    int speed;
    int ret;

    if (count != sizeof(speed))
        return -EINVAL;

    if (copy_from_user(&speed, buf, sizeof(speed)))
        return -EFAULT;

    /* 현재 구현은 OFF/S1/S2/S3 지원 */
    if (speed < 0 || speed > 3)
        return -EINVAL;

    if (mutex_lock_interruptible(&fan_lock))
        return -ERESTARTSYS;

    ret = set_speed(speed);

    mutex_unlock(&fan_lock);

    if (ret)
        return ret;

    pr_info("fan_pwm: speed=%d\n", speed);
    return sizeof(speed);
}

static int fan_release(struct inode *inode, struct file *file)
{
    int ret;

    /* 프로그램이 장치를 닫으면 정지 */
    mutex_lock(&fan_lock);
    ret = set_speed(0);
    mutex_unlock(&fan_lock);

    if (ret)
        pr_err("fan_pwm: PWM OFF failed: %d\n", ret);

    atomic_set(&opened, 0);
    return 0;
}

static const struct file_operations fan_fops = {
    .owner   = THIS_MODULE,
    .open    = fan_open,
    .write   = fan_write,
    .release = fan_release,
    .llseek  = no_llseek,
};

/* /dev/fan_pwm 자동 생성 */
static struct miscdevice fan_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "fan_pwm",
    .fops  = &fan_fops,
};

static int __init fan_pwm_init(void)
{
    int ret;

    if (pwm_id < 0) {
        pr_err("fan_pwm: pwm_id parameter is required\n");
        return -EINVAL;
    }

    ret = gpio_request(GPIO_IN1, "fan_in1");
    if (ret) {
        pr_err("fan_pwm: GPIO request failed: %d\n", ret);
        return ret;
    }

    ret = gpio_direction_output(GPIO_IN1, 0);
    if (ret)
        goto err_gpio;

    motor_pwm = pwm_request(pwm_id, "smartfan-motor");
    if (IS_ERR(motor_pwm)) {
        ret = PTR_ERR(motor_pwm);
        pr_err("fan_pwm: cannot request PWM %d: %d\n",
               pwm_id, ret);
        goto err_gpio;
    }

    /* 초기 상태 OFF */
    mutex_lock(&fan_lock);
    ret = set_speed(0);
    mutex_unlock(&fan_lock);

    if (ret) {
        pr_err("fan_pwm: initial PWM setup failed: %d\n", ret);
        goto err_pwm;
    }

    ret = misc_register(&fan_device);
    if (ret)
        goto err_pwm;

    pr_info("fan_pwm: ready, /dev/fan_pwm, PWM=%d\n",
            pwm_id);

    return 0;

err_pwm:
    gpio_set_value(GPIO_IN1, 0);
    pwm_disable(motor_pwm);
    pwm_free(motor_pwm);

err_gpio:
    gpio_free(GPIO_IN1);
    return ret;
}

static void __exit fan_pwm_exit(void)
{
    misc_deregister(&fan_device);

    mutex_lock(&fan_lock);

    if (set_speed(0))
        pr_err("fan_pwm: PWM OFF failed during unload\n");

    mutex_unlock(&fan_lock);

    gpio_set_value(GPIO_IN1, 0);
    pwm_disable(motor_pwm);
    pwm_free(motor_pwm);
    gpio_free(GPIO_IN1);

    pr_info("fan_pwm: removed\n");
}

module_init(fan_pwm_init);
module_exit(fan_pwm_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Smart Fan motor PWM driver");