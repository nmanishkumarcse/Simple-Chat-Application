/*
 * chat_monitor.c — educational Linux character device.
 *
 * User space (our chat server) can write a short stats text.
 * Anyone can read it:  cat /dev/chat_monitor
 *
 * This driver does NOT implement TCP or chat. It only stores a small buffer.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/version.h>
#include <linux/string.h>

#define DEVICE_NAME "chat_monitor"
#define CLASS_NAME  "chat_monitor"
#define BUF_LEN     512

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SimpleChat student project");
MODULE_DESCRIPTION("Tiny stats device for the chat server");

static dev_t dev_num;
static struct cdev chat_cdev;
static struct class *chat_class;
static struct device *chat_device;
static DEFINE_MUTEX(buf_lock);
static char stats_buf[BUF_LEN] = "status=stopped\nactive_clients=0\ntotal_messages=0\ntotal_connections=0\n";
static size_t stats_len;

static int chat_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int chat_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t chat_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
	ssize_t ret;

	if (mutex_lock_interruptible(&buf_lock))
		return -ERESTARTSYS;

	ret = simple_read_from_buffer(user_buf, count, ppos, stats_buf, stats_len);
	mutex_unlock(&buf_lock);
	return ret;
}

static ssize_t chat_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	size_t n = count;

	if (n >= BUF_LEN)
		n = BUF_LEN - 1;

	if (mutex_lock_interruptible(&buf_lock))
		return -ERESTARTSYS;

	if (copy_from_user(stats_buf, user_buf, n)) {
		mutex_unlock(&buf_lock);
		return -EFAULT;
	}
	stats_buf[n] = '\0';
	stats_len = n;
	mutex_unlock(&buf_lock);

	*ppos = 0;
	return (ssize_t)count;
}

static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = chat_open,
	.release = chat_release,
	.read = chat_read,
	.write = chat_write,
};

static char *chat_devnode(const struct device *dev, umode_t *mode)
{
	if (mode)
		*mode = 0666;
	return NULL;
}

static int __init chat_init(void)
{
	int ret;

	stats_len = strlen(stats_buf);

	ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
	if (ret)
		return ret;

	cdev_init(&chat_cdev, &fops);
	chat_cdev.owner = THIS_MODULE;
	ret = cdev_add(&chat_cdev, dev_num, 1);
	if (ret)
		goto err_region;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	chat_class = class_create(CLASS_NAME);
#else
	chat_class = class_create(THIS_MODULE, CLASS_NAME);
#endif
	if (IS_ERR(chat_class)) {
		ret = PTR_ERR(chat_class);
		goto err_cdev;
	}
	chat_class->devnode = chat_devnode;

	chat_device = device_create(chat_class, NULL, dev_num, NULL, DEVICE_NAME);
	if (IS_ERR(chat_device)) {
		ret = PTR_ERR(chat_device);
		goto err_class;
	}

	pr_info("chat_monitor: loaded, /dev/%s major=%d\n", DEVICE_NAME, MAJOR(dev_num));
	return 0;

err_class:
	class_destroy(chat_class);
err_cdev:
	cdev_del(&chat_cdev);
err_region:
	unregister_chrdev_region(dev_num, 1);
	return ret;
}

static void __exit chat_exit(void)
{
	device_destroy(chat_class, dev_num);
	class_destroy(chat_class);
	cdev_del(&chat_cdev);
	unregister_chrdev_region(dev_num, 1);
	pr_info("chat_monitor: unloaded\n");
}

module_init(chat_init);
module_exit(chat_exit);
