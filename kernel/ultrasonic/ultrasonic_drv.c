#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/completion.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/ktime.h>
#include <linux/delay.h>
#include <linux/jiffies.h>

#include "fan_common.h"

#define GPIO_TRIG 460
#define GPIO_ECHO 461

#define ECHO_TIMEOUT_MS 50
#define MEASURE_INTERVAL_MS 60

struct ultrasonic_device {
    int irq;
    struct miscdevice misc;

    struct mutex measure_lock;
    spinlock_t irq_lock;
    struct completion done;

    bool measuring;
    bool rising_seen;

    s64 start_ns;
    s64 pulse_ns;
};

static struct ultrasonic_device sensor;

/* Echo 상승·하강 인터럽트 처리 */
static irqreturn_t ultrasonic_irq(int irq, void *data)
{
    struct ultrasonic_device *dev = data;
    unsigned long flags;
    s64 now = ktime_get_ns();
    int level = gpio_get_value(GPIO_ECHO);

    spin_lock_irqsave(&dev->irq_lock, flags);

    if (dev->measuring) {
        if (level && !dev->rising_seen) {
            /* 상승: 시간 측정 시작 */
            dev->start_ns = now;
            dev->rising_seen = true;
        } else if (!level && dev->rising_seen) {
            /* 하강: Echo 펄스 길이 계산 */
            dev->pulse_ns = now - dev->start_ns;
            dev->measuring = false;
            complete(&dev->done);
        }
    }

    spin_unlock_irqrestore(&dev->irq_lock, flags);

    return IRQ_HANDLED;
}

static int ultrasonic_open(struct inode *inode, struct file *file)
{
    file->private_data = &sensor;
    return nonseekable_open(inode, file);
}

static ssize_t ultrasonic_read(struct file *file,
                               char __user *buf,
                               size_t count, loff_t *ppos)
{
    struct ultrasonic_device *dev = file->private_data;
    unsigned long flags;
    long waited;
    s64 pulse_ns;
    int distance_mm;
    ssize_t ret;

    if (count < sizeof(distance_mm))
        return -EINVAL;

    /* 여러 프로그램의 동시 측정 방지 */
    if (mutex_lock_interruptible(&dev->measure_lock))
        return -ERESTARTSYS;

    /* 이전 측정과 간격 확보 */
    if (msleep_interruptible(MEASURE_INTERVAL_MS)) {
        ret = -ERESTARTSYS;
        goto out_unlock;
    }

    /* Echo가 이미 HIGH이면 이번 측정은 시작하지 않음 */
    if (gpio_get_value(GPIO_ECHO)) {
        ret = -EBUSY;
        goto out_unlock;
    }

    spin_lock_irqsave(&dev->irq_lock, flags);

    reinit_completion(&dev->done);
    dev->rising_seen = false;
    dev->pulse_ns = 0;
    dev->measuring = true;

    spin_unlock_irqrestore(&dev->irq_lock, flags);

    /* Trigger: LOW → 10us HIGH → LOW */
    gpio_set_value(GPIO_TRIG, 0);
    udelay(2);
    gpio_set_value(GPIO_TRIG, 1);
    udelay(10);
    gpio_set_value(GPIO_TRIG, 0);

    /* 최대 50ms 동안 Echo 완료 대기 */
    waited = wait_for_completion_interruptible_timeout(
        &dev->done,
        msecs_to_jiffies(ECHO_TIMEOUT_MS));

    spin_lock_irqsave(&dev->irq_lock, flags);

    dev->measuring = false;
    pulse_ns = dev->pulse_ns;

    spin_unlock_irqrestore(&dev->irq_lock, flags);

    if (waited < 0) {
        ret = waited;
        goto out_unlock;
    }

    if (waited == 0) {
        ret = -ETIMEDOUT;
        goto out_unlock;
    }

    if (pulse_ns <= 0) {
        ret = -EIO;
        goto out_unlock;
    }

    /*
     * 거리(cm) = Echo 시간(us) / 58
     * 거리(mm) = Echo 시간(ns) / 5800
     */
    distance_mm = (int)(pulse_ns / 5800);

    if (distance_mm <= 0) {
        ret = -EIO;
        goto out_unlock;
    }

    if (copy_to_user(buf, &distance_mm,
                     sizeof(distance_mm))) {
        ret = -EFAULT;
        goto out_unlock;
    }

    /* read() 호출마다 새 거리값 전달 */
    ret = sizeof(distance_mm);

out_unlock:
    mutex_unlock(&dev->measure_lock);
    return ret;
}

static int ultrasonic_release(struct inode *inode,
                              struct file *file)
{
    return 0;
}

static const struct file_operations ultrasonic_fops = {
    .owner   = THIS_MODULE,
    .open    = ultrasonic_open,
    .read    = ultrasonic_read,
    .release = ultrasonic_release,
    .llseek  = no_llseek,
};

static int __init ultrasonic_init(void)
{
    int ret;

    mutex_init(&sensor.measure_lock);
    spin_lock_init(&sensor.irq_lock);
    init_completion(&sensor.done);

    ret = gpio_request(GPIO_TRIG, "ultrasonic_trig");
    if (ret) {
        pr_err("ultrasonic: Trig GPIO request failed: %d\n",
               ret);
        return ret;
    }

    ret = gpio_direction_output(GPIO_TRIG, 0);
    if (ret)
        goto err_trig;

    ret = gpio_request(GPIO_ECHO, "ultrasonic_echo");
    if (ret) {
        pr_err("ultrasonic: Echo GPIO request failed: %d\n",
               ret);
        goto err_trig;
    }

    ret = gpio_direction_input(GPIO_ECHO);
    if (ret)
        goto err_echo;

    /* 인터럽트에서 읽을 수 있는 GPIO인지 확인 */
    if (gpio_cansleep(GPIO_TRIG) ||
        gpio_cansleep(GPIO_ECHO)) {
        ret = -EOPNOTSUPP;
        goto err_echo;
    }

    sensor.irq = gpio_to_irq(GPIO_ECHO);
    if (sensor.irq < 0) {
        ret = sensor.irq;
        goto err_echo;
    }

    ret = request_irq(
        sensor.irq,
        ultrasonic_irq,
        IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
        "ultrasonic_echo",
        &sensor);

    if (ret) {
        pr_err("ultrasonic: IRQ request failed: %d\n", ret);
        goto err_echo;
    }

    /* /dev/ultrasonic 자동 생성 */
    sensor.misc.minor = MISC_DYNAMIC_MINOR;
    sensor.misc.name = "ultrasonic";
    sensor.misc.fops = &ultrasonic_fops;

    ret = misc_register(&sensor.misc);
    if (ret)
        goto err_irq;

    pr_info("ultrasonic: ready, Trig=%d Echo=%d IRQ=%d\n",
            GPIO_TRIG, GPIO_ECHO, sensor.irq);

    return 0;

err_irq:
    free_irq(sensor.irq, &sensor);

err_echo:
    gpio_free(GPIO_ECHO);

err_trig:
    gpio_free(GPIO_TRIG);
    return ret;
}

static void __exit ultrasonic_exit(void)
{
    misc_deregister(&sensor.misc);
    free_irq(sensor.irq, &sensor);

    gpio_set_value(GPIO_TRIG, 0);
    gpio_free(GPIO_ECHO);
    gpio_free(GPIO_TRIG);

    pr_info("ultrasonic: removed\n");
}

module_init(ultrasonic_init);
module_exit(ultrasonic_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Smart Fan HC-SR04 ultrasonic driver");