#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/i2c.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/system.h>


#define DIR_NAME		"test_proc"
#define ENTRY_NAME		"rw_test"
#define DEVICE_NAME		"charp-dev"
#define GLOBALMEM_SIZE	0x1000
#define GLOBALMEM_MAJOR	0

struct globalmem_dev{
	struct cdev cdev;
	unsigned char cdev_mem[GLOBALMEM_SIZE];
	unsigned char proc_mem[GLOBALMEM_SIZE];
};

static int globalmem_major = GLOBALMEM_MAJOR;
static struct proc_dir_entry *test_dir = NULL;
static struct proc_dir_entry *test_entry = NULL;
static struct globalmem_dev *globalmem_devp= NULL;

static int proc_open(struct inode *inode, struct file *filp)
{
	filp->private_data = globalmem_devp;

	return 0;
}

static ssize_t proc_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
	int ret = 0;
	unsigned long count = size;
	unsigned long p = *ppos;
	struct globalmem_dev *dev = filp->private_data;

	if(count > GLOBALMEM_SIZE)
		return -EFAULT;
	if(count > GLOBALMEM_SIZE - p)
		count = GLOBALMEM_SIZE - p;
	
	if(copy_to_user(buf, dev->proc_mem, count))
	{
		return -EINVAL;
	}
	else
	{
		*ppos += count;
		ret = count;
	}
	printk(KERN_INFO "read %ld bytes(s) from %ld\n", count, p);

	return ret;
}

static ssize_t proc_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
	int ret = 0;
	unsigned long count = size;
	unsigned long p = *ppos;
	struct globalmem_dev *dev = filp->private_data;

	if(count > GLOBALMEM_SIZE)
		return -EFAULT;
	if(count > GLOBALMEM_SIZE - p)
		count = GLOBALMEM_SIZE - p;
	
	if(copy_from_user(dev->proc_mem, buf, count))
	{
		return -EINVAL;
	}
	else
	{
		*ppos += count;
		ret = count;
	}
	printk(KERN_INFO "write %ld bytes(s) to %ld\n", count, p);

	return ret;
}

static int proc_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static const struct file_operations proc_fops = {
	.owner			= THIS_MODULE,
	.open			= proc_open,
	.read			= proc_read,
	.write			= proc_write,
	.release		= proc_release,
};

static int globalmem_setup_proc(void)
{
	test_dir = proc_mkdir(DIR_NAME, NULL);
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)/* 从3.10内核后架构有转变 */
	test_entry = create_proc_entry(ENTRY_NAME, 0666, test_dir);
	test_entry->proc_fops = &proc_fops;
#else
	test_entry = proc_create(ENTRY_NAME, 0666, test_dir, &proc_fops);
#endif

	return 0;
}

static int cdev_open(struct inode *inode, struct file *filp)
{
	struct globalmem_dev *dev = NULL;

	dev = container_of(inode->i_cdev, struct globalmem_dev, cdev);
	filp->private_data = dev;

	return 0;
}

static ssize_t cdev_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
	int ret = 0;
	unsigned long count = size;
	unsigned long p = *ppos;
	struct globalmem_dev *dev = filp->private_data;

	if(count > GLOBALMEM_SIZE)
		return -EFAULT;
	if(count > GLOBALMEM_SIZE - p)
		count = GLOBALMEM_SIZE - p;
	
	if(copy_to_user(buf, dev->cdev_mem, count))
	{
		return -EINVAL;
	}
	else
	{
		*ppos += count;
		ret = count;
	}
	printk(KERN_INFO "read %ld bytes(s) from %ld\n", count, p);

	return ret;
}

static ssize_t cdev_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
	int ret = 0;
	unsigned long count = size;
	unsigned long p = *ppos;
	struct globalmem_dev *dev = filp->private_data;

	if(count > GLOBALMEM_SIZE)
		return -EFAULT;
	if(count > GLOBALMEM_SIZE - p)
		count = GLOBALMEM_SIZE - p;
	
	if(copy_from_user(dev->cdev_mem, buf, count))
	{
		return -EINVAL;
	}
	else
	{
		*ppos += count;
		ret = count;
	}
	printk(KERN_INFO "write %ld bytes(s) to %ld\n", count, p);

	return ret;
}

static int cdev_close(struct inode *inode, struct file *filp)
{
	return 0;
}

static const struct file_operations cdev_fops = {
	.owner			= THIS_MODULE,
	.open			= cdev_open,
	.read			= cdev_read,
	.write			= cdev_write,
	.release		= cdev_close,
};

static int globalmem_setup_cdev(struct globalmem_dev * dev, int index)
{
	int ret = 0;
	dev_t devno = MKDEV(globalmem_major, index);

	if(globalmem_major)
	{
		register_chrdev_region(devno, 1, DEVICE_NAME);
	}
	else
	{
		ret = alloc_chrdev_region(&devno, 0, 1, DEVICE_NAME);
		globalmem_major = MAJOR(devno);
	}

	cdev_init(&dev->cdev, &cdev_fops);
	ret = cdev_add(&dev->cdev, devno, 1);
	if(ret)
		printk(KERN_NOTICE "Error %d adding globalmem cdev %d", ret, index);

	return 0;
}

static int __init proc_module_init(void)
{
	globalmem_devp = kmalloc(sizeof(struct globalmem_dev), GFP_KERNEL);
	if(NULL == globalmem_devp)
		goto fail_malloc;

	memset(globalmem_devp, 0, sizeof(struct globalmem_dev));

	globalmem_setup_proc();
	globalmem_setup_cdev(&globalmem_devp[0], 0);
	globalmem_setup_cdev(&globalmem_devp[1], 1);

	return 0;

fail_malloc:
	kfree(globalmem_devp);
	return -EFAULT;
}

static void __exit proc_module_exit(void)
{
	remove_proc_entry(ENTRY_NAME, test_dir);
	remove_proc_entry(DIR_NAME, NULL);
	
	cdev_del(&globalmem_devp[0].cdev);
	cdev_del(&globalmem_devp[1].cdev);
	kfree(globalmem_devp);
	unregister_chrdev_region(MKDEV(globalmem_major, 0), 2);
}

module_init(proc_module_init);
module_exit(proc_module_exit);
MODULE_AUTHOR("kiwei");
MODULE_DESCRIPTION("test proc_file and cdev");
MODULE_LICENSE("GPL");
