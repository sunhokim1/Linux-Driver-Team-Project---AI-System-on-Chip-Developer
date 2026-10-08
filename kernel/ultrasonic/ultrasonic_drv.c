#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>

#include "fan_common.h"

static int ultrasonic_open(struct inode *inode, struct file *file)
{
    (void)inode;
    (void)file;
    /* TODO (Member 1): associate device context with file->private_data. */
    return -ENOSYS;
}

static ssize_t ultrasonic_read(struct file *file, char __user *buf,
                               size_t count, loff_t *ppos)
{
    (void)file;
    (void)buf;
    (void)count;
    (void)ppos;
    /* TODO: check count, trigger pulse, wait for Echo IRQ with timeout.
     * TODO: compute int distance_mm and copy_to_user().
     * TODO: define repeated read behavior and protect shared IRQ data.
     */
    return -ENOSYS;
}

static int ultrasonic_release(struct inode *inode, struct file *file)
{
    (void)inode;
    (void)file;
    /* TODO: release per-open resources. */
    return 0;
}

static const struct file_operations ultrasonic_fops __maybe_unused = {
    .owner = THIS_MODULE,
    .open = ultrasonic_open,
    .read = ultrasonic_read,
    .release = ultrasonic_release,
};

static int __init ultrasonic_init(void)
{
    /* TODO: acquire GPIOs, initialize IRQ/wait queue and synchronization.
     * TODO: register character device and create /dev/ultrasonic.
     * TODO: unwind acquired resources on every failure path.
     */
    pr_info("ultrasonic: skeleton, initialization not implemented\n");
    return -ENOSYS;
}

static void __exit ultrasonic_exit(void)
{
    /* TODO: remove device, stop measurements, release IRQ/GPIO resources. */
}

module_init(ultrasonic_init);
module_exit(ultrasonic_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Smart Fan ultrasonic character driver skeleton");
